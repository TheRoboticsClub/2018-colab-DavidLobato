package examples.spislavedevice

import spinal.core._
import spinal.sim._
import spinal.core.sim._

object SpiSlaveDeviceSim {
  def main(args: Array[String]) {
    SimConfig.withWave.doSim(SpiSlaveDevice(SpiSlaveDeviceConfig())) { dut =>

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

      dut.clockDomain.forkStimulus(period = 10)

      dut.io.spi.ss #= true
      dut.io.spi.sclk #= false
      dut.io.spi.mosi #= false

      dut.clockDomain.waitSampling(10)

      dut.io.spi.ss #= false

      val task1 = readByte(0x00)
      sendByte(0x00)
      task1.join()

      assert(!dut.io.out0.toBoolean)
      assert(!dut.io.out1.toBoolean)
      assert(!dut.io.out2.toBoolean)

      val task2 = readByte(0x00)
      sendByte(0x01)
      task2.join()

      assert(dut.io.out0.toBoolean)
      assert(!dut.io.out1.toBoolean)
      assert(!dut.io.out2.toBoolean)

      val task3 = readByte(0x01)
      sendByte(0x00)

      task3.join()
      assert(!dut.io.out0.toBoolean)
      assert(!dut.io.out1.toBoolean)
      assert(!dut.io.out2.toBoolean)

      dut.io.spi.ss #= true


    }
  }
}
