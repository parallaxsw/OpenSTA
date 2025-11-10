module top (in1, in2, clk1, clk2, clk3, out);
  input in1, in2, clk1, clk2, clk3;
  output out;
  wire r1q, r2q, u1z, u2z;

  DFFHQx4_ASAP7_75t_R r1 (.D(in1), .CLK(clk1), .Q(r1q));
  DFFHQx4_ASAP7_75t_R r2 (.D(in2), .CLK(clk2), .Q(r2q));
  BUFx2_ASAP7_75t_R u1 (.A(r2q), .Y(u1z));
  AND2x2_ASAP7_75t_R u2 (.A(r1q), .B(u1z), .Y(u2z));
  DFFHQx4_ASAP7_75t_R r3 (.D(u2z), .CLK(clk3), .Q(out));

  test_mod u0 (.clk(clk1), .A({u1z, u1z, u1z, u1z}));

endmodule // top


module test_mod(clk, clkout, A, B);
  input clk;
  output clkout;
  input [3:0] A;
  output [3:0] B;

  wire w0;
  wire w1;
  wire w2;
  wire w3;

 BUFx4_ASAP7_75t_R clkbuf0 (.A(clk),
    .Y(clkout));
 BUFx4_ASAP7_75t_R buf0 (.A(A[0]),
    .Y(w0));
 BUFx4_ASAP7_75t_R buf0_1 (.A(w0),
    .Y(B[0]));
 BUFx4_ASAP7_75t_R buf1 (.A(A[1]),
    .Y(w1));    
 BUFx4_ASAP7_75t_R buf1_1 (.A(w1),
    .Y(B[1]));
 BUFx4_ASAP7_75t_R buf2 (.A(A[2]),
    .Y(w2));
 BUFx4_ASAP7_75t_R buf2_1 (.A(w2),
    .Y(B[2]));
 BUFx4_ASAP7_75t_R buf3 (.A(A[3]),
    .Y(w3));    
 BUFx4_ASAP7_75t_R buf3_1 (.A(w3),
    .Y(B[3]));

endmodule
