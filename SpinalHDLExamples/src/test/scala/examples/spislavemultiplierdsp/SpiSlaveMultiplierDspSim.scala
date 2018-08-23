package examples.spislavemultiplierdsp

import spinal.core._
import spinal.core.sim._
import spinal.sim._

object SpiSlaveMultiplierDspSim {
  def main(args: Array[String]) {
    SimConfig
      .withWave
      .addSimulatorFlag("-y /home/dlobato/opt/lscc/radiant/1.0/cae_library/simulation/verilog/iCE40UP")
      .addSimulatorFlag("-Wno-lint")
      .doSim(SpiSlaveMultiplierDsp(SpiSlaveMultiplierDspConfig())) { dut =>

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

      readByte(0x00)
      sendByte(0x02) //OP1

      readByte(0x00)
      sendByte(0x02) //OP2

      readByte(0x00) //RESULTH
      sendByte(0x00) //DUMMY1

      readByte(0x04) //RESULTL
      sendByte(0x00) //DUMMY2

      dut.io.spi.ss #= true
    }
  }
}
