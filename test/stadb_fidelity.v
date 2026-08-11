module top (clk, d, si, se, q);
  input clk, d, si, se;
  output q;
  wire n1;

  BUF u1 (.A(d), .Z(n1));
  DFF r1 (.D(n1), .SI(si), .SE(se), .CK(clk), .Q(q));
endmodule
