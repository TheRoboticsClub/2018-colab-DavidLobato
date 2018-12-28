package examples.multiplier32bit

import spinal.core._
import spinal.lib._

case class Multiplier32bitImpl() {
  def asyncMultiplier(valid: Bool, multiplicand: SInt, multiplier: SInt, ready: Bool, result: SInt): Area = new Area {
    ready := valid
    result := 0
    when(valid) {
      result := multiplicand * multiplier
    }
  }

  def sync1CycleMultiplier(valid: Bool, multiplicand: SInt, multiplier: SInt, ready: Bool, result: SInt): Area = new Area {
    ready := RegNext(valid) init(False)
    result := ready ? RegNextWhen(multiplicand * multiplier, valid) | 0
  }

  def sequentialMultiplier(unrollFactor: Int = 1)(valid: Bool, multiplicand: SInt, multiplier: SInt, ready: Bool, result: SInt): Area = new Area {
    assert(isPow2(unrollFactor))
    val a = Reg(SInt(32 bits)) //multiplicand
    val accumulator = Reg(SInt(64 bits))
    val counter = Counter(32 / unrollFactor + 1)
    ready := counter.willOverflowIfInc
    result := ready ? accumulator | 0

    when(valid) {
      when(valid.rise(False)) {
        // negate operands if multiplier is negative (msb == 1)
        a := multiplier.msb ? -multiplicand | multiplicand
        accumulator := (S(0, 32 bits) @@ (multiplier.msb ? -multiplier | multiplier))
      } elsewhen(!ready) {
        counter.increment()
        val sumElements = ((0 until unrollFactor).map(i => accumulator(i) ? (a << i) | S(0)) :+ accumulator(63 downto 32))
        val sumResult =  sumElements.map(_.resize(32 + unrollFactor)).reduceBalancedTree(_ + _)
        accumulator := (sumResult @@ accumulator(31 downto 0)) >> unrollFactor
      }
    } otherwise {
      counter.clear()
    }

//    assert(isPow2(unrollFactor))
//    val a = Reg(UInt(32 bits)) //multiplicand
//    val x = Reg(UInt(32 bits)) //multiplier
//    val accumulator = Reg(UInt(64 bits)) init(0)
//    val counter = Counter(32 / unrollFactor + 1)
//    ready := counter.willOverflowIfInc
//    result := ready ? accumulator.asSInt | 0

    //    when(valid) {
//      when(valid.rise(False)) {
//        // negate operands if multiplier is negative (msb == 1)
//        a := (multiplier.msb ? -multiplicand | multiplicand).asUInt
//        x := (multiplier.msb ? -multiplier | multiplier).asUInt
//        accumulator := 0
//      } elsewhen(!ready) {
//        counter.increment()
//        x := x |>> unrollFactor
//        val sumElements = ((0 until unrollFactor).map(i => x(i) ? (a << i) | U(0)) :+ accumulator(63 downto 32))
//        val sumResult =  sumElements.map(_.asSInt.resize(32 + unrollFactor).asUInt).reduceBalancedTree(_ + _)
//        accumulator := (sumResult @@ accumulator(31 downto 0)) >> unrollFactor
//      }
//    } otherwise {
//      counter.clear()
//    }
  }
}


case class Multiplier32bit(multImpl: (Bool, SInt, SInt, Bool, SInt) => Area) extends Component {
  val io = new Bundle {
    val valid = in Bool
    val multiplicand = in SInt(32 bits)
    val multiplier = in SInt(32 bits)

    val ready = out Bool
    val result = out SInt(64 bits)
  }

  val impl = multImpl(io.valid, io.multiplicand, io.multiplier, io.ready, io.result)
}

object Multiplier32bit {
  def main(args: Array[String]) {
    val outRtlDir = if (!args.isEmpty) args(0) else  "rtl"
    SpinalConfig(
      targetDirectory = outRtlDir,
      defaultClockDomainFrequency = FixedFrequency(12 MHz),
      defaultConfigForClockDomains = ClockDomainConfig(
        resetKind = BOOT
      )
    ).generateVerilog(Multiplier32bit(Multiplier32bitImpl().sequentialMultiplier(1)))
  }
}