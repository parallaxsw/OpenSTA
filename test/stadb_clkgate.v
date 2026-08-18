// Minimal netlist that instantiates an integrated clock-gating cell so
// liberty restore must preserve is_clock_gate (derived from port attrs).
module top (clk, en, d, q);
  input clk, en, d;
  output q;
  wire gclk;

  CLKGATE_X1 cg (.CK(clk), .E(en), .GCK(gclk));
  DFF_X1 r1 (.D(d), .CK(gclk), .Q(q));
endmodule
