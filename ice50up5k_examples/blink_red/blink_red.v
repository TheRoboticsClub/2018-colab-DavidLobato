module top(
  input clk,
  output RGB0, RGB1, RGB2
);

parameter c_DELAY_1S_COUNT = 6000000; // 500ms @ 12MHz

reg r_State = 1'b0;
reg [22:0] r_Count = 0;

always@(posedge clk)
begin
  if (r_Count < c_DELAY_1S_COUNT)
    r_Count <= r_Count + 1;
  else
    begin
      r_Count <= 0;
      r_State <= ~r_State;
    end
  
end

SB_RGBA_DRV RGBA_DRIVER (
  .CURREN(1'b1),
  .RGBLEDEN(1'b1),
  .RGB0PWM(1'b0),
  .RGB1PWM(1'b0),
  .RGB2PWM(r_State),
  .RGB0(RGB0),
  .RGB1(RGB1),
  .RGB2(RGB2)
);


defparam RGBA_DRIVER.CURRENT_MODE = "0b1";
defparam RGBA_DRIVER.RGB0_CURRENT = "0b000111";
defparam RGBA_DRIVER.RGB1_CURRENT = "0b000111";
defparam RGBA_DRIVER.RGB2_CURRENT = "0b000111";


endmodule
