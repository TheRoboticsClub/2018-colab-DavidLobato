package examples.spislavemultiplierdsp

import examples.blackbox.lattice.ice40._
import spinal.core._
import spinal.lib._
import spinal.lib.com.spi.{SpiSlave, SpiSlaveCtrl, SpiSlaveCtrlGenerics}
import spinal.lib.io.InOutWrapper

// mode: 0 => [cpol = 0, cpha = 0], 1 => [cpol = 0, cpha = 1], 2 => [cpol = 1, cpha = 0], 3 => [cpol = 1, cpha = 1]
case class SpiSlaveMultiplierDspConfig(dataWidth : Int = 8, mode: Int = 0)

object SpiSlaveMultiplierDspState extends SpinalEnum {
  val OPERAND1, OPERAND2, RESULTH, RESULTL = newElement()
}

case class SpiSlaveMultiplierDsp(config: SpiSlaveMultiplierDspConfig) extends Component {
  val io = new Bundle{
    val spi = master(SpiSlave())
  }

  val coreCtrl = new Area {
    val op1 = Reg(UInt(config.dataWidth bits)) init(0)
    val op2 = Reg(UInt(config.dataWidth bits)) init(0)
    val result = UInt(config.dataWidth*2 bits)
    val mac16 = SB_MAC16(SB_MAC16_Config(
      topOutputSelect = OutputSelectEnum.MUL_8x8,
      bottomOutputSelect = OutputSelectEnum.MUL_8x8,
      mode8x8 = true
    ))

    mac16.C := 0
    mac16.A := op1.asBits.resized
    mac16.B := op2.asBits.resized
    mac16.D := 0

    mac16.AHOLD := False
    mac16.BHOLD := False
    mac16.CHOLD := False
    mac16.DHOLD := False

    mac16.IRSTTOP := False
    mac16.IRSTBOT := False
    mac16.ORSTTOP := False
    mac16.ORSTBOT := False

    mac16.OLOADTOP := False
    mac16.OLOADBOT := False
    mac16.ADDSUBTOP := False
    mac16.ADDSUBBOT := False
    mac16.OHOLDTOP := False
    mac16.OHOLDBOT := False
    mac16.CI := False
    mac16.ACCUMCI := False
    mac16.SIGNEXTIN := False

    result := mac16.O.asUInt.resized

    val txData = Bits(config.dataWidth bits)

    val spiCtrl = new SpiSlaveCtrl(SpiSlaveCtrlGenerics(config.dataWidth))
    spiCtrl.io.kind.cpha := Bool(config.mode == 1 || config.mode == 3)
    spiCtrl.io.kind.cpol := Bool(config.mode == 2 || config.mode == 3)

    var rxLogic = new Area {
      import SpiSlaveMultiplierDspState._

      val state = RegInit(OPERAND1)

      txData := B(0)

      switch(state) {
        is(OPERAND1) {
          when(spiCtrl.io.rx.fire) {
            op1 := spiCtrl.io.rx.payload.asUInt
            state := OPERAND2
          }
        }
        is(OPERAND2) {
          when(spiCtrl.io.rx.fire) {
            op2 := spiCtrl.io.rx.payload.asUInt
            state := RESULTH
          }
        }
        is(RESULTH) {
          txData := result.asBits.subdivideIn(config.dataWidth bits)(1)
          when(spiCtrl.io.rx.fire) {
            state := RESULTL
          }
        }
        is(RESULTL) {
          txData := result.asBits.subdivideIn(config.dataWidth bits)(0)
          when(spiCtrl.io.rx.fire) {
            state := OPERAND1
          }
        }
      }

      when(spiCtrl.io.ssFilted) {
        state := OPERAND1
      }
    }

    //tx logic
    spiCtrl.io.tx.payload := txData
  }

  io.spi <> coreCtrl.spiCtrl.io.spi

  noIoPrefix()
}

object SpiSlaveMultiplierDsp {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(
      targetDirectory = outRtlDir,
      defaultClockDomainFrequency = FixedFrequency(12 MHz),
      defaultConfigForClockDomains = ClockDomainConfig(
        resetKind = BOOT
      )
    ).generateVerilog(InOutWrapper(SpiSlaveMultiplierDsp(SpiSlaveMultiplierDspConfig())))
  }
}