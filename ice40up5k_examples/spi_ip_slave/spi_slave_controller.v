module spi_slave_controller (
    output wire MISO, 
    input wire MOSI, 
    input wire SCK,
    input wire SS,

    input wire RST,
    output wire IPDONE,
    input wire SBCLKi,
    input wire SBWRi, 
    //  This bus is available when IPDONE = 1
    input wire SBSTBi, 
    input wire [7:0] SBADRi, 
    input wire [7:0] SBDATi, 
    output wire [7:0] SBDATo, 
    output wire SBACKo, 
        
    output wire IRQ, 
    output wire PWKUP
);

    // params
    
    // SM states
    localparam LOAD = 0;
    localparam WAITACK = 1;
    localparam DONE = 2;

    // wires
    wire hard_SBWRi;
    wire hard_SBSTBi;
    wire [7:0] hard_SBADRi;
    wire [7:0] hard_SBDATi;
    wire ssm_SBWRi;
    wire ssm_SBSTBi;
    wire [7:0] ssm_SBADRi;
    wire [7:0] ssm_SBDATi;
    wire hard_SBACKO;
    wire [7:0] hard_SBDATo;
    wire [15:0] init_seq_data;
    wire end_of_seq;

    // regs
    reg strobe;
    reg load_next;
    reg IPDONE_i;
    reg [3:0] init_seq_addr;
    reg[1:0] state, next_state;

    rom8x16 ROM (
      .clk(SBCLKi),
      .addr(init_seq_addr),
      .data(init_seq_data)
    );

    always @(posedge SBCLKi or negedge RST) begin
      if (!RST) state <= LOAD;
      else state <= next_state;
    end

    always @(posedge SBCLKi or negedge RST) begin
      if (!RST) init_seq_addr <= 0;
      else if (load_next) init_seq_addr <= init_seq_addr + 1;
    end

    always @(*) begin
      next_state = state;
      IPDONE_i = 0;
      strobe = 0;
      load_next = 0;

      case (state)
        LOAD: begin
          if (!hard_SBACKO) next_state = WAITACK;
        end
        WAITACK: begin
          if (end_of_seq) next_state = DONE;
          else begin
            strobe = 1;
            if (hard_SBACKO) begin
              next_state = LOAD;
              load_next = 1;
            end
          end
        end
        DONE: begin
          next_state = DONE;
          IPDONE_i = 1;
        end
      endcase
    end

    // Output assignments
    assign ssm_SBWRi = 1; // All transaction are WRITE
    assign ssm_SBSTBi = strobe;
    assign ssm_SBADRi = init_seq_data[15:8];
    assign ssm_SBDATi = init_seq_data[7:0];
    assign end_of_seq = & init_seq_data[15:8];//end_of_seq when data=0xFFxx
    assign SBACKo = (hard_SBACKO && IPDONE_i); 
    assign IPDONE = IPDONE_i;
    assign MISO = (SPI_SOoe ? SPI_SO : 1'bz);
    assign hard_SBWRi = (IPDONE_i ? SBWRi : ssm_SBWRi);
    assign hard_SBSTBi = (IPDONE_i ? SBSTBi : ssm_SBSTBi);
    assign hard_SBADRi = (IPDONE_i ? SBADRi : ssm_SBADRi);
    assign hard_SBDATi = (IPDONE_i ? SBDATi : ssm_SBDATi);

    SB_SPI #(.BUS_ADDR74("0b0000")) SB_SPI_INST (
        .SBCLKI(SBCLKi), 
        .SBRWI(hard_SBWRi),
        .SBSTBI(hard_SBSTBi),
        .SBADRI7(hard_SBADRi[7]),
        .SBADRI6(hard_SBADRi[6]),
        .SBADRI5(hard_SBADRi[5]),
        .SBADRI4(hard_SBADRi[4]),
        .SBADRI3(hard_SBADRi[3]),
        .SBADRI2(hard_SBADRi[2]),
        .SBADRI1(hard_SBADRi[1]),
        .SBADRI0(hard_SBADRi[0]),
        .SBDATI7(hard_SBDATi[7]),
        .SBDATI6(hard_SBDATi[6]),
        .SBDATI5(hard_SBDATi[5]),
        .SBDATI4(hard_SBDATi[4]), 
        .SBDATI3(hard_SBDATi[3]),
        .SBDATI2(hard_SBDATi[2]),
        .SBDATI1(hard_SBDATi[1]),
        .SBDATI0(hard_SBDATi[0]),
        .MI(),
        .SI(MOSI),
        .SCKI(SCK),
        .SCSNI(SS),
        .SBDATO7(SBDATo[7]),
        .SBDATO6(SBDATo[6]),
        .SBDATO5(SBDATo[5]),
        .SBDATO4(SBDATo[4]),
        .SBDATO3(SBDATo[3]),
        .SBDATO2(SBDATo[2]),
        .SBDATO1(SBDATo[1]), 
        .SBDATO0(SBDATo[0]),
        .SBACKO(hard_SBACKO),
        .SPIIRQ(IRQ),
        .SPIWKUP(PWKUP),
        .SO(SPI_SO),
        .SOE(SPI_SOoe),
        .MO(),
        .MOE(),
        .SCKO(),
        .SCKOE(),
        .MCSNO3(),
        .MCSNO2(),
        .MCSNO1(),
        .MCSNO0(),
        .MCSNOE3(), 
        .MCSNOE2(),
        .MCSNOE1(),
        .MCSNOE0()
    );

endmodule