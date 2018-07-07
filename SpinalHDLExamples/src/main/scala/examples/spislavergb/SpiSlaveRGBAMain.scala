package examples.spislavergb

import spinal.core._
import spinal.lib.io.InOutWrapper

object SpiSlaveRGBAMain {
  def main(args: Array[String]) {
    SpinalConfig(targetDirectory = "rtl")
      .generateVerilog(InOutWrapper(SpiSlaveRGBA(SpiSlaveRGBAConfig())))
      .printPruned()
  }
}
