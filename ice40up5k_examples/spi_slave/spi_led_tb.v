(* blackbox *)
module SB_RGBA_DRV(
        input CURREN,
        input RGBLEDEN,
        input RGB0PWM,
        input RGB1PWM,
        input RGB2PWM,
        output RGB0,
        output RGB1,
        output RGB2
);
parameter CURRENT_MODE = "0b0";
parameter RGB0_CURRENT = "0b000000";
parameter RGB1_CURRENT = "0b000000";
parameter RGB2_CURRENT = "0b000000";
endmodule

module spi_led_tb();

reg clk;
reg ss;
reg mosi;
wire miso;
reg sck;
reg[7:0] misoData;
reg[7:0] expectedData;

spi_led dut (
  .clk(clk),
  .RGB0(),
  .RGB1(),
  .RGB2(),
  .SS(ss),
  .SCK(sck),
  .MOSI(mosi),
  .MISO(miso)
);

// clock generator
always 
    # 1 clk <= ~clk;

task send_data;
    input [7:0] data;
    output [7:0] misoData;
begin
    ss = 1'b0;
    #10 mosi = data[0]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 0
    #10 mosi = data[1]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 1
    #10 mosi = data[2]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 2
    #10 mosi = data[3]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 3
    #10 mosi = data[4]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 4
    #10 mosi = data[5]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 5
    #10 mosi = data[6]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 6
    #10 mosi = data[7]; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 7
    #10 ss = 1'b1;
end
endtask


initial begin
    $dumpfile("spi_led_tb.vcd");
    $dumpvars(0, spi_led_tb);

    clk = 1'b0;
    sck = 1'b0;
    mosi = 1'b0;
    ss = 1'b1;
    misoData = 8'h00;

    expectedData = 8'b11111111;
    #10 send_data(8'b11111111, misoData);
    $display("assertEquals(misoData,0x%h,0x%h)", 8'b10101010, misoData);
    
    #100 $display("END");
    $finish;
end

endmodule