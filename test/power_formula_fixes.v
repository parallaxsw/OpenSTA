module power_formula_fixes (clk, en, te, a, b, q0, q1, y);
  input clk, en, te, a, b;
  output q0, q1, y;
  wire gclk0, gclk1, n1;
  ICGx1_ASAP7_75t_R cg0 (.CLK(clk), .ENA(en), .SE(1'b0), .GCLK(gclk0));
  DFFHQx4_ASAP7_75t_R r0 (.CLK(gclk0), .D(a), .Q(q0));
  ICGx1_ASAP7_75t_R cg1 (.CLK(clk), .ENA(en), .SE(te), .GCLK(gclk1));
  DFFHQx4_ASAP7_75t_R r1 (.CLK(gclk1), .D(a), .Q(q1));
  BUFx2_ASAP7_75t_R u1 (.A(a), .Y(n1));
  AND2x2_ASAP7_75t_R u2 (.A(n1), .B(b), .Y(y));
endmodule
