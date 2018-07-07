package examples.spislavedevice

import spinal.core._
import spinal.lib._
import spinal.lib.com.spi.{SpiSlave, SpiSlaveCtrl, SpiSlaveCtrlGenerics}

// mode: 0 => [cpol = 0, cpha = 0], 1 => [cpol = 0, cpha = 1], 2 => [cpol = 1, cpha = 0], 3 => [cpol = 1, cpha = 1]
case class SpiSlaveDeviceConfig(dataWidth : Int = 8, mode: Int = 0)

case class SpiSlaveDevice(config: SpiSlaveDeviceConfig) extends Component {
  val io = new Bundle{
    val spi = master(SpiSlave())

    //Peripherals IO
    val out0, out1, out2 = out Bool()
  }

  val coreCtrl = new Area {
    val outState = Reg(Bits(3 bits)) init(0)

    val spiCtrl = new SpiSlaveCtrl(SpiSlaveCtrlGenerics(config.dataWidth))
    spiCtrl.io.kind.cpha := Bool(config.mode == 1 || config.mode == 3)
    spiCtrl.io.kind.cpol := Bool(config.mode == 2 || config.mode == 3)

    //tx logic
    spiCtrl.io.tx.valid := True
    spiCtrl.io.tx.payload := outState.resized

    //rx logic
    when(spiCtrl.io.rx.fire) {
      outState := spiCtrl.io.rx.payload.resized
    }
  }

  io.out0 := coreCtrl.outState(0)
  io.out1 := coreCtrl.outState(1)
  io.out2 := coreCtrl.outState(2)

  io.spi <> coreCtrl.spiCtrl.io.spi
}
