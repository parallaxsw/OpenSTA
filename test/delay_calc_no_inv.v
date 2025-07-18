module delay_calc_no_inv (
  input in,
  output out
);

  wire mid;

  BUFx2_ASAP7_75t_R I1 (
    .A(in),
    .Y(mid)
  );

  INVx2_ASAP7_75t_R I2 (
    .A(mid),
    .Y(out)
  );

endmodule