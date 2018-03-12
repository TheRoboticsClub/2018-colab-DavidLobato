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


spi_slave spi_slave_instance (
  .clk(clk),
  .rst(rst),
  .ss(ss),
  .mosi(mosi),
  .miso(miso),
  .sck(sck),
  .done(done),
  .din(8'b10101010),
  .dout(receivedData)
);

// clock generator
always 
    # 1 clk <= ~clk;


//-- Proceso al inicio
initial begin

    //-- Fichero donde almacenar los resultados
    $dumpfile("spi_slave_tb.vcd");
    $dumpvars(0, spi_slave_tb);

    clk = 1'b0;
    sck = 1'b0;
    mosi = 1'b0;
    ss = 1'b1;
    misoData = 8'h00;
  
    #10 ss = 1'b0;

    expectedData = 8'b11111111;
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 0
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 1
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 2
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 3
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 4
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 5
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 6
    #10 mosi = 1'b1; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 7
    #10
    $display("assertEquals(misoData,0x%h,0x%h)", 8'b10101010, misoData);
    
    expectedData = 8'b00000000;
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 0
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 1
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 2
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 3
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 4
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 5
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 6
    #10 mosi = 1'b0; #10 sck = 1'b0; #10 sck = 1'b1; #10 misoData <= { misoData[6:0], miso }; // bit 7
    #10
    $display("assertEquals(misoData,0x%h,0x%h)", 8'b10101010, misoData);

    #100 $display("FIN de la simulacion");
    $finish;
end

always @(*) begin
    if(done)
        $display("assertEquals(receivedData,%b,%b)", receivedData, expectedData);
end

endmodule