module block (clk, in, out);
  input clk;
  input in;
  output out;
  wire n1, n2;
  BUF_X1 u1 (.A(in), .Z(n1));
  DFF_X1 r1 (.D(n1), .CK(clk), .Q(n2));
  BUF_X1 u2 (.A(n2), .Z(out));
endmodule

module block_rc (clk, in, out);
  input clk;
  input in;
  output out;
  wire n1, n2;
  BUF_X1 u1 (.A(in), .Z(n1));
  DFF_X1 r1 (.D(n1), .CK(clk), .Q(n2));
  BUF_X1 u2 (.A(n2), .Z(out));
endmodule

module parent (clk);
  input clk;
  wire n;
  block_model b1 (.clk(clk), .in(1'b0), .out(n));
  block_model b2 (.clk(clk), .in(n), .out());
endmodule
