package examples.blinkred

import examples.blackbox.lattice.ice40._
import spinal.core._
import spinal.lib.Timeout

case class BlinkRed(period: TimeNumber = 0.1 sec) extends Component{
  val io = new Bundle{
    val rgb0,rgb1,rgb2 = out Bool()
  }

  val timeout = Timeout(period)
  when(timeout) {
    timeout.clear()
  }

  val state = Reg(Bool)
  when(timeout) {
    state := !state
  }

  val rgbaDriverConfig = SB_RGBA_DRV_Config(
    currentMode = "0b1",
    rgb0Current = "0b000011",
    rgb1Current = "0b000011",
    rgb2Current = "0b000011"
  )
  val rgbaDriver = SB_RGBA_DRV(rgbaDriverConfig)

  rgbaDriver.CURREN := True
  rgbaDriver.RGBLEDEN := True
  rgbaDriver.RGB0PWM := False
  rgbaDriver.RGB1PWM := False
  rgbaDriver.RGB2PWM := state
  rgbaDriver.RGB0 <> io.rgb0
  rgbaDriver.RGB1 <> io.rgb1
  rgbaDriver.RGB2 <> io.rgb2

  noIoPrefix()
}

object BlinkRed {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(
      targetDirectory = outRtlDir,
      defaultClockDomainFrequency = FixedFrequency(12 MHz),
      defaultConfigForClockDomains = ClockDomainConfig(
        resetKind = BOOT
      )
    ).generateVerilog(BlinkRed())
  }
}