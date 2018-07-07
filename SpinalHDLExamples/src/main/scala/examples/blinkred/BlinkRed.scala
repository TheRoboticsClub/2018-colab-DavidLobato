package examples.blinkred

import examples.sbrgbadrv.{SB_RGBA_DRV_Config, SB_RGBA_DRV}
import spinal.core._

case class BlinkRed(timerWidth: Int = 23) extends Component{
  val io = new Bundle{
    val rgb0,rgb1,rgb2 = out Bool()
  }

  val timer = new Area {
    val counter = Reg(UInt(timerWidth bits))
    val tick = counter === 0
    counter := counter - 1
    when(tick) {
      counter := 6000000 //500ms @ 12MHz
    }
  }

  val state = Reg(Bool)
  when(timer.tick) {
    state := !state
  }

  val rgbaDriverConfig = SB_RGBA_DRV_Config(
    currentMode = "0b1",
    rgb0Current = "0b000011",
    rgb1Current = "0b000011",
    rgb2Current = "0b000011"
  )
  val rgbaDriver = SB_RGBA_DRV(rgbaDriverConfig)

  rgbaDriver.io.CURREN := True
  rgbaDriver.io.RGBLEDEN := True
  rgbaDriver.io.RGB0PWM := state
  rgbaDriver.io.RGB1PWM := False
  rgbaDriver.io.RGB2PWM := False
  rgbaDriver.io.RGB0 <> io.rgb0
  rgbaDriver.io.RGB1 <> io.rgb1
  rgbaDriver.io.RGB2 <> io.rgb2
}
