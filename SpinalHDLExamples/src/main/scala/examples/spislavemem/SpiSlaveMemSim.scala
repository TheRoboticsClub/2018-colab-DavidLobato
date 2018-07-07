package examples.spislavemem

import spinal.core._
import spinal.sim._
import spinal.core.sim._

object SpiSlaveMemSim {
  def main(args: Array[String]) {
    SimConfig.withWave.doSim(SpiSlaveMem(SpiSlaveMemConfig(mode = 0, memWordCount = 256))) { dut =>
      dut.resetCtrlClockDomain.forkStimulus(period = 10)

      //dut.io.reset #= false
      dut.io.spi.ss #= true
      dut.io.spi.sclk #= false
      dut.io.spi.mosi #= false

      dut.resetCtrlClockDomain.waitSampling(100) //reset takes 64 cycles

      def sendByte(data: Int) = {
        (dut.config.dataWidth - 1 downto 0).suspendable.foreach { bitId =>
          dut.io.spi.sclk #= true
          dut.io.spi.mosi #= ((data >> bitId) & 1) != 0
          sleep(100)
          dut.io.spi.sclk #= false
          sleep(100)
        }
      }

      def readByte(expectedData: Int) = {
        fork{
          var buffer = 0x00
          (dut.config.dataWidth - 1 downto 0).suspendable.foreach { bitId =>
            //mode 0 sample data on sclk leading edge
            waitUntil(dut.io.spi.sclk.toBoolean)
            if (dut.io.spi.miso.writeEnable.toBoolean && dut.io.spi.miso.write.toBoolean) buffer |= 1 << bitId
            waitUntil(!dut.io.spi.sclk.toBoolean)
          }
          assert(expectedData == buffer, "expected data=0x" + expectedData.toHexString + ", received data=0x" + buffer.toHexString)
        }
      }

      //write sequence 0..255 to memory
      dut.io.spi.ss #= false
      readByte(0x00)
      sendByte(0x80) // WRITE

      readByte(0x00)
      sendByte(0x00) // ADDR 0

      (0 until dut.config.memWordCount).suspendable.foreach { idx =>
        readByte(0xFF) //expected initialized data 0xFF
        sendByte(idx) // WRITE BYTE
      }
      dut.io.spi.ss #= true
      sleep(100)

      //read memory, expected sequence 0..255
      dut.io.spi.ss #= false
      readByte(0x00)
      sendByte(0x00) // READ

      readByte(0x00)
      sendByte(0x00) // ADDR 0

      (0 until dut.config.memWordCount).suspendable.foreach { idx =>
        readByte(idx)
        sendByte(idx)
      }
      dut.io.spi.ss #= true
      sleep(100)
    }
  }
}
