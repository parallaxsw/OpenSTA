module top (in, clk1, clk2, out, out2);
   input in, clk1, clk2;
   output out, out2;
   block1 b1 (.in(in), .clk(clk1), .out(b1out), .out2(out2));
   block2 b2 (.in(b1out), .clk(clk2), .out(out));
endmodule // top

module block1 (in, clk, out, out2);
   input in, clk;
   output out, out2;
   BUF_X1 u1 (.A(in), .Z(u1out));
   DFF_X1 r1 (.D(u1out), .CK(clk), .Q(r1q));
   BUF_X1 u2 (.A(r1q), .Z(out));
   BUF_X1 u3 (.A(out), .Z(out2));
endmodule // block1

module block2 (in, clk, out, out2);
   input in, clk;
   output out, out2;
   BUF_X1 u1 (.A(in), .Z(u1out));
   DFF_X1 r1 (.D(u1out), .CK(clk), .Q(r1q));
   BUF_X1 u2 (.A(r1q), .Z(out));
   BUF_X1 u3 (.A(out), .Z(out2));
endmodule // block2



