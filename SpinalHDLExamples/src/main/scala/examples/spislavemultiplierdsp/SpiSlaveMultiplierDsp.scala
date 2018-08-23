package examples.spislavemultiplierdsp

import examples.sbmac16.{OutputSelectEnum, SB_MAC16, SB_MAC16_Config}
import spinal.core._
import spinal.lib._
import spinal.lib.com.spi.{SpiSlave, SpiSlaveCtrl, SpiSlaveCtrlGenerics}
import spinal.lib.io.InOutWrapper

// mode: 0 => [cpol = 0, cpha = 0], 1 => [cpol = 0, cpha = 1], 2 => [cpol = 1, cpha = 0], 3 => [cpol = 1, cpha = 1]
case class SpiSlaveMultiplierDspConfig(dataWidth : Int = 8, mode: Int = 0)

object SpiSlaveMultiplierDspState extends SpinalEnum {
  val OPERAND1, OPERAND2, RESULTL, RESULTH = newElement()
}

case class SpiSlaveMultiplierDsp(config: SpiSlaveMultiplierDspConfig) extends Component {
  val io = new Bundle{
    val spi = master(SpiSlave())
  }

  val coreCtrl = new Area {
    val mac16 = SB_MAC16(SB_MAC16_Config(
      aReg = true,
      bReg = true,
      topOutputSelect = OutputSelectEnum.MUL_8x8,
      bottomOutputSelect = OutputSelectEnum.MUL_8x8,
      mode8x8 = true
    ))

    mac16.io.CLK := ClockDomain.current.readClockWire
    mac16.io.CE := True

    mac16.io.C := 0
    mac16.io.A := 0
    mac16.io.B := 0
    mac16.io.D := 0

    mac16.io.AHOLD := True
    mac16.io.BHOLD := True
    mac16.io.CHOLD := False
    mac16.io.DHOLD := False

    mac16.io.IRSTTOP := False
    mac16.io.IRSTBOT := False
    mac16.io.ORSTTOP := False
    mac16.io.ORSTBOT := False

    mac16.io.OLOADTOP := False
    mac16.io.OLOADBOT := False
    mac16.io.ADDSUBTOP := False
    mac16.io.ADDSUBBOT := False
    mac16.io.OHOLDTOP := False
    mac16.io.OHOLDBOT := False
    mac16.io.CI := False
    mac16.io.ACCUMCI := False
    mac16.io.SIGNEXTIN := False

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
            mac16.io.A := spiCtrl.io.rx.payload.resized
            mac16.io.AHOLD := False //load
            state := OPERAND2
          }
        }
        is(OPERAND2) {
          when(spiCtrl.io.rx.fire) {
            mac16.io.B := spiCtrl.io.rx.payload.resized
            mac16.io.BHOLD := False //load
            state := RESULTL
          }
        }
        is(RESULTH) {
          txData := mac16.io.O.subdivideIn(config.dataWidth bits)(1)
          state := RESULTL
        }
        is(RESULTL) {
          txData := mac16.io.O.subdivideIn(config.dataWidth bits)(0)
          state := OPERAND1
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
    ).generateVerilog(InOutWrapper(SpiSlaveMultiplierDsp(SpiSlaveMultiplierDspConfig()))).printPruned()
  }
}