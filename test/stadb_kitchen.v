module top (clk, d, q);
  input clk;
  input [1:0] d;
  output [1:0] q;
  wire n0, n1;
  BUF u0 (.A(d[0]), .Z(n0));
  BUF u1 (.A(d[1]), .Z(n1));
  DFF r0 (.D(n0), .CK(clk), .Q(q[0]));
  DFF r1 (.D(n1), .CK(clk), .Q(q[1]));
endmodule
