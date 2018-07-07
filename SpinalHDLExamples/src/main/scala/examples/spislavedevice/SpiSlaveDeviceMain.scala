package examples.spislavedevice

import spinal.core._
import spinal.lib.io.InOutWrapper

object SpiSlaveDeviceMain {
  def main(args: Array[String]) {
    SpinalConfig(targetDirectory = "rtl")
      .generateVerilog(InOutWrapper(SpiSlaveDevice(SpiSlaveDeviceConfig())))
      .printPruned()
  }
}
