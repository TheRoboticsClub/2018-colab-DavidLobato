package examples.blinkred

import spinal.core._

object BlinkRedMain {
  def main(args: Array[String]) {
    SpinalConfig(targetDirectory = "rtl").generateVerilog(BlinkRed())
  }
}
