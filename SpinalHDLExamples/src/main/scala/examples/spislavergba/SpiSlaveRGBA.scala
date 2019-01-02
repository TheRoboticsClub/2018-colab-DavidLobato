package examples.spislavergba

import examples.blackbox.lattice.ice40._
import spinal.core._
import spinal.lib._
import spinal.lib.com.spi.{SpiSlave, SpiSlaveCtrl, SpiSlaveCtrlGenerics}
import spinal.lib.io.InOutWrapper

// mode: 0 => [cpol = 0, cpha = 0], 1 => [cpol = 0, cpha = 1], 2 => [cpol = 1, cpha = 0], 3 => [cpol = 1, cpha = 1]
case class SpiSlaveRGBAConfig(dataWidth : Int = 8, mode: Int = 0)

case class SpiSlaveRGBA(config: SpiSlaveRGBAConfig) extends Component {
  val io = new Bundle{
    val clk    = in Bool

    val spi = master(SpiSlave())

    //Peripherals IO
    val rgb0,rgb1,rgb2 = out Bool()
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

  val rgbaDriverConfig = SB_RGBA_DRV_Config(
    currentMode = "0b1",
    rgb0Current = "0b000011",
    rgb1Current = "0b000011",
    rgb2Current = "0b000011"
  )
  val rgbaDriver = SB_RGBA_DRV(rgbaDriverConfig)

  val core = new ClockingArea(coreClockDomain) {
    val rgbState = Reg(Bits(3 bits)) init(0)

    val spiCtrl = new SpiSlaveCtrl(SpiSlaveCtrlGenerics(config.dataWidth))
    spiCtrl.io.kind.cpha := Bool(config.mode == 1 || config.mode == 3)
    spiCtrl.io.kind.cpol := Bool(config.mode == 2 || config.mode == 3)

    //tx logic
    spiCtrl.io.tx.valid := True
    spiCtrl.io.tx.payload := rgbState.resized

    //rx logic
    when(spiCtrl.io.rx.valid) {
      rgbState := spiCtrl.io.rx.payload.resized
    }
  }

  rgbaDriver.CURREN := core.rgbState.orR
  rgbaDriver.RGBLEDEN := core.rgbState.orR
  rgbaDriver.RGB0PWM := core.rgbState(0)
  rgbaDriver.RGB1PWM := core.rgbState(1)
  rgbaDriver.RGB2PWM := core.rgbState(2)

  io.rgb0 <> rgbaDriver.RGB0
  io.rgb1 <> rgbaDriver.RGB1
  io.rgb2 <> rgbaDriver.RGB2

  io.spi <> core.spiCtrl.io.spi

  noIoPrefix()
}

object SpiSlaveRGBA {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(targetDirectory = outRtlDir)
      .generateVerilog(InOutWrapper(SpiSlaveRGBA(SpiSlaveRGBAConfig())))
  }
}