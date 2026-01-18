module dut (
  input in1,
  input in2,
  input clk1,
  input clk2,
  input clk3,
  output out
);

  wire w_mid1, w_mid2;

  blk_stage1 u_blk1 (
    .in1(in1),
    .in2(in2),
    .clk1(clk1),
    .clk2(clk2),
    .mid1(w_mid1),
    .mid2(w_mid2)
  );

  blk_stage2 u_blk2 (
    .mid1(w_mid1),
    .mid2(w_mid2),
    .clk3(clk3),
    .out(out)
  );

endmodule


module blk_stage1 (
  input in1,
  input in2,
  input clk1,
  input clk2,
  output mid1,
  output mid2
);

  wire r1q, r2q, u1z;

  DFF_X1 blk_r1 (
    .D(in1),
    .CK(clk1),
    .Q(r1q)
  );

  DFF_X1 blk_r2 (
    .D(in2),
    .CK(clk2),
    .Q(r2q)
  );

  BUF_X1 blk_u1 (
    .A(r2q),
    .Z(u1z)
  );

  assign mid1 = r1q;
  assign mid2 = u1z;

endmodule


module blk_stage2 (
  input mid1,
  input mid2,
  input clk3,
  output out
);

  wire u2z;

  AND2_X1 blk_u2 (
    .A1(mid1),
    .A2(mid2),
    .ZN(u2z)
  );

  DFF_X1 blk_r3 (
    .D(u2z),
    .CK(clk3),
    .Q(out)
  );

endmodule