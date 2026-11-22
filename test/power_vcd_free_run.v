// 2-bit free-running counter: the clock is the only input.
module power_vcd_free_run (clk, q0, q1);
  input clk;
  output q0, q1;
  wire d0, d1, qn0, qn1;
  DFF_X1 r0 (.D(d0), .CK(clk), .Q(q0), .QN(qn0));
  DFF_X1 r1 (.D(d1), .CK(clk), .Q(q1), .QN(qn1));
  INV_X1 u0 (.A(q0), .ZN(d0));
  XOR2_X1 u1 (.A(q0), .B(q1), .Z(d1));
endmodule
