module top (in, clk, out);
  input in, clk;
  output out;
  wire r1q, l1q;

  DFFHQx4_ASAP7_75t_R r1 (.D(in), .CLK(clk), .Q(r1q));
  DHLx1_ASAP7_75t_R   l1 (.D(r1q), .CLK(clk), .Q(l1q));
  DFFHQx4_ASAP7_75t_R r2 (.D(l1q), .CLK(clk), .Q(out));
endmodule
