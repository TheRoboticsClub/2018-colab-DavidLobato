`default_nettype none
//from https://github.com/Obijuan/open-fpga-verilog-tutorial/wiki/Cap%C3%ADtulo-26%3A-Memoria-ROM

module rom8x16 (input clk,
                input wire [3:0] addr,
                output reg [15:0] data);

  reg [15:0] rom [0:7];

  always @(posedge clk) begin
    data <= rom[addr];
  end

  initial begin
    // addr + value
    rom[0] = {8'b00000111, 8'b00000000};//SPIIRQEN
    rom[1] = {8'b00001111, 8'b00000001};//SPICSR
    rom[2] = {8'b00001001, 8'b10000000};//SPICR1
    rom[3] = {8'b00001010, 8'b00000000};//SPICR2
    rom[4] = {8'b00001011, 8'b00001011};//SPIBR
    rom[5] = {8'b11111111, 8'b00000000};//addr=FF marks end of config registers
   end
endmodule