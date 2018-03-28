module SB_SPI (
        input  SBCLKI,
        input  SBRWI,                              
        input  SBSTBI,
        input  SBADRI7,
        input  SBADRI6,
        input  SBADRI5,
        input  SBADRI4,
        input  SBADRI3,
        input  SBADRI2,
        input  SBADRI1,
        input  SBADRI0,
        input  SBDATI7,
        input  SBDATI6,
        input  SBDATI5,
        input  SBDATI4,
        input  SBDATI3,
        input  SBDATI2,
        input  SBDATI1,
        input  SBDATI0,
        input  MI,
        input  SI,
        input  SCKI,
        input  SCSNI,
        output SBDATO7,
        output SBDATO6,
        output SBDATO5,
        output SBDATO4,
        output SBDATO3,
        output SBDATO2,
        output SBDATO1,
        output SBDATO0,
        output SBACKO,
        output SPIIRQ,
        output SPIWKUP,
        output SO,
        output SOE,
        output MO,
        output MOE,
        output SCKO, //inout in the SB verilog library, but output in the VHDL and PDF libs and seemingly in the HW itself
        output SCKOE,
        output MCSNO3,
        output MCSNO2,
        output MCSNO1,
        output MCSNO0,
        output MCSNOE3,
        output MCSNOE2,
        output MCSNOE1,
        output MCSNOE0
);
parameter BUS_ADDR74 = "0b0000";

reg SBACKO;
reg SBSTBI_old = 0;
reg ack = 0;

always @(posedge SBCLKI) begin
    SBSTBI_old <= SBSTBI;
    if (!SBSTBI_old && SBSTBI) begin
        ack <= 1;
    end
    SBACKO <= ack;
end

always @(posedge SBCLKI) begin
    if (ack) ack <= 0;
end

endmodule

module spi_slave_controller_tb();

reg clk;
reg sck;
reg ss;
reg mosi;
reg rst;
wire miso;

spi_slave_controller dut(
    .MISO(miso),
    .MOSI(mosi),
    .SCK(sck),
    .SS(ss),
    .RST(rst),
    .IPDONE(),
    .SBCLKi(clk),
    .SBWRi(),
    .SBSTBi(),
    .SBADRi(),
    .SBDATi(),
    .SBDATo(),
    .SBACKo(),
    .IRQ(),
    .PWKUP()
);

// clock generator
always 
    # 1 clk <= ~clk;

initial begin
    $dumpfile("spi_slave_controller_tb.vcd");
    $dumpvars(0, spi_slave_controller_tb);

    clk = 1'b0;
    sck = 1'b0;
    mosi = 1'b0;
    ss = 1'b1;
    rst = 1'b0;

    #5 rst = ~rst;

    #500 $display("END");
    $finish;
end

endmodule