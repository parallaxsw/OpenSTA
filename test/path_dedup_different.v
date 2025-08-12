module esoteric_design(
	input CLK,
	input[3:0] d,
	output[3:0] store
);
	sky130_fd_sc_hd__dfxtp_1 _0_ (
		.CLK(CLK),
		.D(d[0]),
		.Q(store[0])
	);
	
	wire store_1_inv;
	sky130_fd_sc_hd__dfxtp_1 _1_ (
		.CLK(CLK),
		.D(d[1]),
		.Q(store_1_inv)
	);
	sky130_fd_sc_hd__inv_1 _1_inv(
		.Y(store[1]),
		.A(store_1_inv)
	);

	sky130_fd_sc_hd__dfxtp_1 _2_ (
		.CLK(CLK),
		.D(d[2]),
		.Q(store[2])
	);

	sky130_fd_sc_hd__dfxtp_1 _3_ (
		.CLK(CLK),
		.D(d[3]),
		.Q(store[3])
	);
endmodule
