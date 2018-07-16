package examples.spislavedevice

import spinal.core._
import spinal.lib._
import spinal.lib.com.spi.{SpiSlave, SpiSlaveCtrl, SpiSlaveCtrlGenerics}
import spinal.lib.io.InOutWrapper

// mode: 0 => [cpol = 0, cpha = 0], 1 => [cpol = 0, cpha = 1], 2 => [cpol = 1, cpha = 0], 3 => [cpol = 1, cpha = 1]
case class SpiSlaveDeviceConfig(dataWidth : Int = 8, mode: Int = 0)

case class SpiSlaveDevice(config: SpiSlaveDeviceConfig) extends Component {
  val io = new Bundle{
    val spi = master(SpiSlave())

    //Peripherals IO
    val output = out Bits(8 bits)
  }

  val coreCtrl = new Area {
    val outputState = Reg(Bits(io.output.getWidth bits)) init(0)

    val spiCtrl = new SpiSlaveCtrl(SpiSlaveCtrlGenerics(config.dataWidth))
    spiCtrl.io.kind.cpha := Bool(config.mode == 1 || config.mode == 3)
    spiCtrl.io.kind.cpol := Bool(config.mode == 2 || config.mode == 3)

    //tx logic
    spiCtrl.io.tx.valid := True
    spiCtrl.io.tx.payload := outputState.resized

    //rx logic
    when(spiCtrl.io.rx.fire) {
      outputState := spiCtrl.io.rx.payload.resized
    }
  }

  io.output := coreCtrl.outputState

  io.spi <> coreCtrl.spiCtrl.io.spi
}

object SpiSlaveDevice {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(targetDirectory = outRtlDir)
      .generateVerilog(InOutWrapper(SpiSlaveDevice(SpiSlaveDeviceConfig())))
  }
}