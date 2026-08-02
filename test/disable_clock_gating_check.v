module top (clk, en, d, q);
  input clk;
  input en;
  input d;
  output q;
  wire gclk;

  AND2 cg (.A(clk), .B(en), .Z(gclk));
  DFF  ff (.CK(gclk), .D(d), .Q(q));
endmodule
