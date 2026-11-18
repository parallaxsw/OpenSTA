module top (clk, en, d, q1, q2);
  input clk, en, d;
  output q1, q2;
  wire gclk1, se_init;

  CLKGATETST_X1 icg1 (.CK(clk), .E(en), .SE(se_init), .GCK(gclk1));
  DFF_X1 r1 (.CK(gclk1), .D(d), .Q(q1));
endmodule
