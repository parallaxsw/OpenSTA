module dual_edge_test (
  input clk,
  input a,
  input b,
  output z
);
  NAND2 nand2_inst (.a(a), .b(b), .z(z));
endmodule
