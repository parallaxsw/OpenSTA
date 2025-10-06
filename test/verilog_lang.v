
(* src = "synthesis/tests/counter.v:16.1-32.10" *)
module counter(clk, reset, in, out);
  input clk;
  output out;
  input reset;
  input in;
  wire mid;

  parameter PARAM1=1;
  parameter PARAM2="test";

  specify
    specparam SPARAM1=2;
    specparam SPARAM2="test2";
  endspecify

  defparam _1415_.PARAM2 = 1;

  sky130_fd_sc_hd__dfrtp_1 _1415_ (
    .CLK(clk),
    .D(in),
    .Q(mid),
    .RESET_B(reset)
  );

  sky130_fd_sc_hd__dfrtp_1 \_1416_[0] (
    .CLK(clk),
    .D(mid),
    .Q(out),
    .RESET_B(reset)
  );
endmodule
