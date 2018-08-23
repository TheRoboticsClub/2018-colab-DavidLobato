package examples.multiply8x8

import spinal.core._
import spinal.core.sim._
import spinal.sim._

object Multiply8x8Sim {
  def main(args: Array[String]) {
    val simConfig = SimConfig.withWave
      .addSimulatorFlag("-y /home/dlobato/opt/lscc/radiant/1.0/cae_library/simulation/verilog/iCE40UP")
      .addSimulatorFlag("-Wno-lint")

    simConfig.doSim(Multiply8x8Unsigned()) { dut =>

        dut.clockDomain.forkStimulus(period = 10)

        dut.io.aTop #= 2
        dut.io.bTop #= 2

        dut.io.aBottom #= 4
        dut.io.bBottom #= 4

        dut.clockDomain.waitSampling(10)

        assert(dut.io.outputTop.toInt == 4)
        assert(dut.io.outputBottom.toInt == 16)
    }

    simConfig.doSim(Multiply8x8Signed()) { dut =>

        dut.clockDomain.forkStimulus(period = 10)

        dut.io.aTop #= 2
        dut.io.bTop #= -2

        dut.io.aBottom #= 4
        dut.io.bBottom #= -4

        dut.clockDomain.waitSampling(10)

        assert(dut.io.outputTop.toInt == -4)
        assert(dut.io.outputBottom.toInt == -16)
      }
  }
}
