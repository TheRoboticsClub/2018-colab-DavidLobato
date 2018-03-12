module spi_led(
  input wire clk,
  output wire RGB0,
  output wire RGB1,
  output wire RGB2,
  output wire SS,
  output wire SCK,
  output wire MOSI, 
  output wire MISO
);

wire spi_done;
wire [7:0] spi_dout;
reg [7:0] dout;
reg rst = 1'b1;
//reg [7:0] din;

spi_slave SPI1 (
  .clk(clk),
  .rst(rst),
  .ss(SS),
  .mosi(MOSI),
  .miso(MISO),
  .sck(SCK),
  .done(spi_done),
  .din(8'b10101010),
  .dout(spi_dout)
);

SB_RGBA_DRV RGBA_DRIVER (
  .CURREN(1'b1),
  .RGBLEDEN(1'b1),
  .RGB0PWM(dout[2]),
  .RGB1PWM(dout[1]),
  .RGB2PWM(dout[0]),
  .RGB0(RGB0),
  .RGB1(RGB1),
  .RGB2(RGB2)
);

always @(posedge clk) begin
  if (rst) begin
    rst = 1'b0;
  end
  if (spi_done) begin
    dout <= spi_dout;
  end
end


defparam RGBA_DRIVER.CURRENT_MODE = "0b1";
defparam RGBA_DRIVER.RGB0_CURRENT = "0b000111";
defparam RGBA_DRIVER.RGB1_CURRENT = "0b000111";
defparam RGBA_DRIVER.RGB2_CURRENT = "0b000111";


endmodule