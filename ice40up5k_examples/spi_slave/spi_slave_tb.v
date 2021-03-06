module spi_slave_tb();

reg clk;
reg rst;
reg ss;
reg mosi;
wire miso;
reg sck;
wire done;
wire [7:0] receivedData;
reg[7:0] misoData;
reg[7:0] dataToSend;
reg[7:0] expectedData;


spi_slave dut (
  .clk(clk),
  .rst(rst),
  .ss(ss),
  .mosi(mosi),
  .miso(miso),
  .sck(sck),
  .done(done),
  .din(dataToSend),
  .dout(receivedData)
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
    $dumpfile("spi_slave_tb.vcd");
    $dumpvars(0, spi_slave_tb);

    clk = 1'b0;
    sck = 1'b0;
    mosi = 1'b0;
    ss = 1'b1;
    misoData = 8'h00;

    dataToSend = 8'b10101010;//slave output

    expectedData = 8'b11111111;
    #10 send_data(8'b11111111, misoData);
    $display("assertEquals(misoData,0x%h,0x%h)", 8'b10101010, misoData);
    
    dataToSend = 8'b01010101;//slave output
    expectedData = 8'b00000000;
    #10 send_data(8'b00000000, misoData);
    $display("assertEquals(misoData,0x%h,0x%h)", 8'b01010101, misoData);

    #100 $display("END");
    $finish;
end

always @(*) begin
    if(done)
        $display("assertEquals(receivedData,%b,%b)", receivedData, expectedData);
end

endmodule