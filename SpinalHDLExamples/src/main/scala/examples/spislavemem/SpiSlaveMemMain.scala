package examples.spislavemem

import spinal.core._
import spinal.lib.io.InOutWrapper

object SpiSlaveMemMain {
  def main(args: Array[String]) {
    SpinalConfig(targetDirectory = "rtl")
      .generateVerilog(InOutWrapper(SpiSlaveMem(SpiSlaveMemConfig(memWordCount = 1024))))
      .printPruned()
  }
}
