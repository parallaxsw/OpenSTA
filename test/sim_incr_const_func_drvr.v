module top (clk, en, se, d, q1, q2);
  input clk, en, se, d;
  output q1, q2;
  wire gclk1, gclk2;

  CLKGATETST_X1 icg1 (.CK(clk), .E(en), .SE(se), .GCK(gclk1));
  CLKGATETST_X1 icg2 (.CK(clk), .E(en), .SE(se), .GCK(gclk2));
  DFF_X1 r1 (.CK(gclk1), .D(d), .Q(q1));
  DFF_X1 r2 (.CK(gclk2), .D(d), .Q(q2));
endmodule
