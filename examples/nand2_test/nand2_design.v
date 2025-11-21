module NAND2_TOP (
  input clk,
  input A,
  input B,
  output Z
);

  NAND2 nand2_inst (
    .A(A),
    .B(B),
    .Z(Z)
  );

endmodule
