module top (a, z);
  input a;
  output z;
  wire n1, n2;
  AND2_X1 u1 (.A1(a), .A2(n2), .ZN(n1));
  BUF_X1 u2 (.A(n1), .Z(n2));
  BUF_X1 u3 (.A(n1), .Z(z));
endmodule
