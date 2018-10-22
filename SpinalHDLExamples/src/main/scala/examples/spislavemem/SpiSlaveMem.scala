package examples.spislavemem

import spinal.core._
import spinal.lib._
import spinal.lib.com.spi.{SpiSlave, SpiSlaveCtrl, SpiSlaveCtrlGenerics}
import spinal.lib.io.InOutWrapper

// mode: 0 => [cpol = 0, cpha = 0], 1 => [cpol = 0, cpha = 1], 2 => [cpol = 1, cpha = 0], 3 => [cpol = 1, cpha = 1]
case class SpiSlaveMemConfig(dataWidth : Int = 8, mode: Int = 0, memWordCount: Int, initRam: Boolean = false) {
  val addressWidth = log2Up(memWordCount)
  assert(addressWidth < (dataWidth*2), "max memWordCount=32768")
  assert(isPow2(memWordCount), "memWordCount must be a power of 2")
}

object SpiSlaveMemState extends SpinalEnum {
  val CMDH, CMDL, DATA, STOP = newElement()
}

case class SpiSlaveMem(config: SpiSlaveMemConfig) extends Component {
  val io = new Bundle{
    val clk    = in Bool

    val spi = master(SpiSlave())
  }

  val resetCtrlClockDomain = ClockDomain(
    clock = io.clk,
    config = ClockDomainConfig(
      resetKind = BOOT
    )
  )

  val resetCtrl = new ClockingArea(resetCtrlClockDomain) {
    val coreResetUnbuffered = False

    // reset after 64 cycles
    val resetCounter = Reg(UInt(6 bits)) init(0)
    when(resetCounter =/= U(resetCounter.range -> true)){
      resetCounter := resetCounter + 1
      coreResetUnbuffered := True
    }

    val coreReset = RegNext(coreResetUnbuffered)
  }

  val coreClockDomain = ClockDomain(
    clock = io.clk,
    reset = resetCtrl.coreReset
  )

  val core = new ClockingArea(coreClockDomain) {
    val mem =
      if (config.initRam) Mem(initialContent = for(idx <- 0 until config.memWordCount) yield { B(config.dataWidth bits, default -> true) }) //init word all bits 1
      else Mem(Bits(config.dataWidth bits), config.memWordCount)
    val op = Reg(Bool) init(False) // 0 -> READ, 1 -> WRITE
    val enable = Bool
    val writeEnable = Bool
    val address = Reg(UInt(mem.addressWidth bits)) init(0)
    val writeData = Bits(config.dataWidth bits)
    val readData = mem.readWriteSync(
      address = address,
      data = writeData,
      enable = enable,
      write = writeEnable
    )
    val txData = Bits(config.dataWidth bits)

    val spiCtrl = new SpiSlaveCtrl(SpiSlaveCtrlGenerics(config.dataWidth))
    spiCtrl.io.kind.cpha := Bool(config.mode == 1 || config.mode == 3)
    spiCtrl.io.kind.cpol := Bool(config.mode == 2 || config.mode == 3)


    var rxLogic = new Area {
      import SpiSlaveMemState._

      val state = RegInit(CMDH)

      txData := B(0)
      enable := False
      writeEnable := False

      switch(state) {
        is(CMDH) {
          when(spiCtrl.io.rx.fire) {
            op := spiCtrl.io.rx.payload.msb
            address := spiCtrl.io.rx.payload((address.getWidth-config.dataWidth)-1 downto 0).resizeLeft(address.getWidth).asUInt
            state := CMDL
          }
        }
        is(CMDL) {
          when(spiCtrl.io.rx.fire) {
            address := (address((address.getWidth-config.dataWidth)-1 downto 0) ## spiCtrl.io.rx.payload).asUInt
            state := DATA
          }
        }
        is(DATA) {
          enable := True
          txData := readData

          when(spiCtrl.io.rx.fire) {
            writeEnable := op
            address := address + 1
          }
        }
      }

      when(spiCtrl.io.ssFilted) {
        op := False
        state := CMDH
      }

      writeData := spiCtrl.io.rx.payload
    }

    //tx logic
    spiCtrl.io.tx.payload := txData

  }

  io.spi <> core.spiCtrl.io.spi

  noIoPrefix()
}

object SpiSlaveMem {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(targetDirectory = outRtlDir)
      .generateVerilog(InOutWrapper(SpiSlaveMem(SpiSlaveMemConfig(memWordCount = 1024, initRam = true))))
  }
}

object SpiSlaveMemSPRAM {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(targetDirectory = outRtlDir)
      .generateVerilog({
        val toplevel = SpiSlaveMem(SpiSlaveMemConfig(memWordCount = 1024))
        toplevel.core.mem.generateAsBlackBox()
        InOutWrapper(toplevel)
      })
  }
}