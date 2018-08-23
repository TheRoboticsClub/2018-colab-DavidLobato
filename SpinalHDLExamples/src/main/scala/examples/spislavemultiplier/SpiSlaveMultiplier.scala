package examples.spislavemultiplier

import spinal.core._
import spinal.lib._
import spinal.lib.com.spi.{SpiSlave, SpiSlaveCtrl, SpiSlaveCtrlGenerics}
import spinal.lib.io.InOutWrapper

// mode: 0 => [cpol = 0, cpha = 0], 1 => [cpol = 0, cpha = 1], 2 => [cpol = 1, cpha = 0], 3 => [cpol = 1, cpha = 1]
case class SpiSlaveMultiplierConfig(dataWidth : Int = 8, mode: Int = 0)

object SpiSlaveMultiplierState extends SpinalEnum {
  val OPERAND1, OPERAND2, RESULTL, RESULTH = newElement()
}

case class SpiSlaveMultiplier(config: SpiSlaveMultiplierConfig) extends Component {
  val io = new Bundle{
    val spi = master(SpiSlave())
  }

  val coreCtrl = new Area {
    val op1 = Reg(SInt(config.dataWidth bits)) init(0)
    val result = Reg(SInt(config.dataWidth*2 bits)) init(0)
    val txData = Bits(config.dataWidth bits)

    val spiCtrl = new SpiSlaveCtrl(SpiSlaveCtrlGenerics(config.dataWidth))
    spiCtrl.io.kind.cpha := Bool(config.mode == 1 || config.mode == 3)
    spiCtrl.io.kind.cpol := Bool(config.mode == 2 || config.mode == 3)

    var rxLogic = new Area {
      import SpiSlaveMultiplierState._

      val state = RegInit(OPERAND1)

      txData := B(0)

      switch(state) {
        is(OPERAND1) {
          when(spiCtrl.io.rx.fire) {
            op1 := spiCtrl.io.rx.payload.asSInt
            state := OPERAND2
          }
        }
        is(OPERAND2) {
          when(spiCtrl.io.rx.fire) {
            result := op1 * spiCtrl.io.rx.payload.asSInt
            state := RESULTL
          }
        }
        is(RESULTL) {
          txData := result.asBits.subdivideIn(config.dataWidth bits)(0)
          state := RESULTH
        }
        is(RESULTH) {
          txData := result.asBits.subdivideIn(config.dataWidth bits)(1)
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

object SpiSlaveMultiplier {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(
      targetDirectory = outRtlDir,
      defaultClockDomainFrequency = FixedFrequency(12 MHz),
      defaultConfigForClockDomains = ClockDomainConfig(
        resetKind = BOOT
      )
    ).generateVerilog(InOutWrapper(SpiSlaveMultiplier(SpiSlaveMultiplierConfig())))
  }
}