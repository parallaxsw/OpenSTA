module top (a, z);
  inout a;
  output z;
  BUF_X1 u1 (.A(a), .Z(z));
endmodule
