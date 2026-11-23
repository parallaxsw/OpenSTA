module power_clock_gate_test_enable (clk, en, te, gclk0, gclk1);
  input clk, en, te;
  output gclk0, gclk1;
  ICGx1_ASAP7_75t_R cg0 (.CLK(clk), .ENA(en), .SE(1'b0), .GCLK(gclk0));
  ICGx1_ASAP7_75t_R cg1 (.CLK(clk), .ENA(en), .SE(te), .GCLK(gclk1));
endmodule
