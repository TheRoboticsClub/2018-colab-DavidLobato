`timescale 1ps / 1ps

module Ram_1wrs (clk, en, wr, addr, wdata, rdata);

    parameter wordCount = 4096;
    parameter wordWidth = 32;
    parameter readUnderWrite = "dontCare";
    parameter technology = "auto";

    input clk, en, wr;
    input [14:0] addr;
    input [wordWidth-1:0] wdata;
    output [wordWidth-1:0] rdata;

    wire cs_0, cs_1;
    wire [wordWidth-1:0] rdata_0, rdata_1;

    assign cs_0 = !addr[14];
    assign cs_1 = addr[14];
    assign rdata = addr[14] ? rdata_1 : rdata_0;


    SB_SPRAM256KA ram00
                  (
                      .ADDRESS(addr[13:0]),
                      .DATAIN(wdata[15:0]),
                      .MASKWREN({wen[1], wen[1], wen[0], wen[0]}),
                      .WREN(wen[1]|wen[0]),
                      .CHIPSELECT(cs_0),
                      .CLOCK(clk),
                      .STANDBY(1'b0),
                      .SLEEP(1'b0),
                      .POWEROFF(1'b1),
                      .DATAOUT(rdata_0[15:0])
                  );

    SB_SPRAM256KA ram01
                  (
                      .ADDRESS(addr[13:0]),
                      .DATAIN(wdata[31:16]),
                      .MASKWREN({wen[3], wen[3], wen[2], wen[2]}),
                      .WREN(wen[3]|wen[2]),
                      .CHIPSELECT(cs_0),
                      .CLOCK(clk),
                      .STANDBY(1'b0),
                      .SLEEP(1'b0),
                      .POWEROFF(1'b1),
                      .DATAOUT(rdata_0[31:16])
                  );


    SB_SPRAM256KA ram10
                  (
                      .ADDRESS(addr[13:0]),
                      .DATAIN(wdata[15:0]),
                      .MASKWREN({wen[1], wen[1], wen[0], wen[0]}),
                      .WREN(wen[1]|wen[0]),
                      .CHIPSELECT(cs_1),
                      .CLOCK(clk),
                      .STANDBY(1'b0),
                      .SLEEP(1'b0),
                      .POWEROFF(1'b1),
                      .DATAOUT(rdata_1[15:0])
                  );

    SB_SPRAM256KA ram11
                  (
                      .ADDRESS(addr[13:0]),
                      .DATAIN(wdata[31:16]),
                      .MASKWREN({wen[3], wen[3], wen[2], wen[2]}),
                      .WREN(wen[3]|wen[2]),
                      .CHIPSELECT(cs_1),
                      .CLOCK(clk),
                      .STANDBY(1'b0),
                      .SLEEP(1'b0),
                      .POWEROFF(1'b1),
                      .DATAOUT(rdata_1[31:16])
                  );

endmodule