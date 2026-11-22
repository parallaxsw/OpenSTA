module power_slew_limit (clk, en, a, gclk, y);
  input clk, en, a;
  output gclk, y;
  ICGx1_ASAP7_75t_R cg (.CLK(clk), .ENA(en), .SE(1'b0), .GCLK(gclk));
  BUFx2_ASAP7_75t_R u1 (.A(a), .Y(y));
endmodule
