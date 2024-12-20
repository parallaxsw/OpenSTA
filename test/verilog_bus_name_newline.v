module top (in1, in2, out);
  input in1, in2;
  output out;
  wire [1:0] \aaa.aaa ;

  BUFx2_ASAP7_75t_R u1 (.A(in1), .Y(\aaa.aaa [0]));
  BUFx2_ASAP7_75t_R u2 (.A(in2), .Y(\aaa.aaa [1]));
  AND2x2_ASAP7_75t_R u3 (.A(\aaa.aaa [0]), .B(\aaa.aaa
              [1]), .Y(out));
endmodule
