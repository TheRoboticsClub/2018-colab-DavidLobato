package examples.multiply8x8

import examples.sbmac16._
import spinal.core._

case class Multiply8x8(signed: Boolean = false) extends Component {
  val io = new Bundle{
    val aTop = in Bits(8 bits)
    val bTop = in Bits(8 bits)
    val aBottom = in Bits(8 bits)
    val bBottom = in Bits(8 bits)
    val outputTop = out Bits(16 bits)
    val outputBottom = out Bits(16 bits)
  }

  val multiplier = new Area {
    val mac16 = SB_MAC16(SB_MAC16_Config(
      topOutputSelect = OutputSelectEnum.MUL_8x8,
      bottomOutputSelect = OutputSelectEnum.MUL_8x8,
      mode8x8 = true,
      aSigned = signed,
      bSigned = signed
    ))
    val aTopReg = RegNext(io.aTop) init(0)
    val bTopReg = RegNext(io.bTop) init(0)
    val aBottomReg = RegNext(io.aBottom) init(0)
    val bBottomReg = RegNext(io.bBottom) init(0)
    val outputReg = RegNext(mac16.io.O) init(0)

    mac16.io.CLK := ClockDomain.current.readClockWire
    mac16.io.CE := True

    mac16.io.C := 0
    mac16.io.A := aTopReg ## aBottomReg
    mac16.io.B := bTopReg ## bBottomReg
    mac16.io.D := 0

    mac16.io.AHOLD := False
    mac16.io.BHOLD := False
    mac16.io.CHOLD := False
    mac16.io.DHOLD := False

    mac16.io.IRSTTOP := False
    mac16.io.IRSTBOT := False
    mac16.io.ORSTTOP := False
    mac16.io.ORSTBOT := False

    mac16.io.OLOADTOP := False
    mac16.io.OLOADBOT := False
    mac16.io.ADDSUBTOP := False
    mac16.io.ADDSUBBOT := False
    mac16.io.OHOLDTOP := False
    mac16.io.OHOLDBOT := False
    mac16.io.CI := False
    mac16.io.ACCUMCI := False
    mac16.io.SIGNEXTIN := False
  }

  io.outputTop := multiplier.outputReg.subdivideIn(16 bits)(1)
  io.outputBottom := multiplier.outputReg.subdivideIn(16 bits)(0)
}

case class Multiply8x8Unsigned() extends Component {
  val io = new Bundle{
    val aTop = in UInt(8 bits)
    val bTop = in UInt(8 bits)
    val aBottom = in UInt(8 bits)
    val bBottom = in UInt(8 bits)
    val outputTop = out UInt(16 bits)
    val outputBottom = out UInt(16 bits)
  }

  val multiplier = Multiply8x8()
  multiplier.io.aTop := io.aTop.asBits
  multiplier.io.bTop := io.bTop.asBits
  multiplier.io.aBottom := io.aBottom.asBits
  multiplier.io.bBottom := io.bBottom.asBits
  io.outputTop := multiplier.io.outputTop.asUInt
  io.outputBottom := multiplier.io.outputBottom.asUInt
}

case class Multiply8x8Signed() extends Component {
  val io = new Bundle{
    val aTop = in SInt(8 bits)
    val bTop = in SInt(8 bits)
    val aBottom = in SInt(8 bits)
    val bBottom = in SInt(8 bits)
    val outputTop = out SInt(16 bits)
    val outputBottom = out SInt(16 bits)
  }

  val multiplier = Multiply8x8(signed = true)
  multiplier.io.aTop := io.aTop.asBits
  multiplier.io.bTop := io.bTop.asBits
  multiplier.io.aBottom := io.aBottom.asBits
  multiplier.io.bBottom := io.bBottom.asBits
  io.outputTop := multiplier.io.outputTop.asSInt
  io.outputBottom := multiplier.io.outputBottom.asSInt
}