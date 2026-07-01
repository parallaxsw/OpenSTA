module top (
  input wire clk,
  input wire a,
  input wire b,
  output wire z
);

  nand2 u1 (
    .A(a),
    .B(b),
    .Z(z)
  );

endmodule
