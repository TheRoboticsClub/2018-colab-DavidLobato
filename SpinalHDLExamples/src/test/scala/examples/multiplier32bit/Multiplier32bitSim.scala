package examples.multiplier32bit

import spinal.core._
import spinal.core.sim._

import scala.util.Random

object Multiplier32bitSim {
  def main(args: Array[String]) {
    val simConfig = SimConfig.withWave

    simConfig.doSim(Multiplier32bit(Multiplier32bitImpl().asyncMultiplier)) { dut =>

      dut.io.valid #= true

      var idx = 0
      while(idx < 100){
        val a, b = Random.nextInt()
        dut.io.multiplicand #= a
        dut.io.multiplier #= b
        sleep(1)
        assert(dut.io.result.toLong == (a.toLong * b.toLong))
        assert(dut.io.ready.toBoolean)
        idx += 1
      }
    }

    simConfig.doSim(Multiplier32bit(Multiplier32bitImpl().sync1CycleMultiplier)) { dut =>

      dut.clockDomain.forkStimulus(period = 10)

      dut.io.valid #= false

      dut.clockDomain.waitSampling(10)

      var idx = 0
      while(idx < 100){
        val a, b = Random.nextInt()
        dut.io.valid #= true
        dut.io.multiplicand #= a
        dut.io.multiplier #= b
        waitUntil(dut.io.ready.toBoolean)
        assert(dut.io.result.toLong == (a.toLong * b.toLong))
        dut.io.valid #= false
        waitUntil(!dut.io.ready.toBoolean)
        idx += 1
      }
    }

    simConfig.doSim(Multiplier32bit(Multiplier32bitImpl().sequentialMultiplier(2))) { dut =>

      dut.clockDomain.forkStimulus(period = 10)

      dut.io.valid #= false

      dut.clockDomain.waitSampling(10)

      var idx = 0
      while(idx < 100){
        val a, b = Random.nextInt()
        dut.io.valid #= true
        dut.io.multiplicand #= a
        dut.io.multiplier #= b
        waitUntil(dut.io.ready.toBoolean)
        assert(dut.io.result.toLong == (a.toLong * b.toLong),
          "Expected " + a + "*" + b + "=" + (a.toLong * b.toLong) + ", got=" + dut.io.result.toLong)
        dut.io.valid #= false
        waitUntil(!dut.io.ready.toBoolean)
        idx += 1
      }
    }
  }
}
