module verilog_module ();

endmodule

module inst_props (
	input           CLK,
	input           CEN,
	input           GWEN,
	input   [7:0]   WEN,
	input   [6:0]   A,
	input   [7:0]   D,
	output  [7:0]   Q
);

	wire CEN_buf;
	wire CLK_INV;
	
	wire GWEN_reg0;
	wire GWEN_reg1;
	wire GWEN_reg;
	
	verilog_module verilog_module_inst ();

	sky130_fd_sc_hd__buf_2 buf_inst (
		.A(CEN),
		.X(CEN_buf)
	);

	sky130_fd_sc_hd__dfrtp_2 dff_inst0 (
		.CLK(CLK),
		.D(GWEN),
		.Q(GWEN_reg0)
	);
	
	sky130_fd_sc_hd__clkinv_2 clk_inv (
		.A(CLK),
		.Y(CLK_INV)
	);
	
	sky130_fd_sc_hd__dfrtn_1 dff_inst1 (
		.CLK_N(CLK_INV),
		.D(GWEN),
		.Q(GWEN_reg1)
	);
	
	sky130_fd_sc_hd__and2_2 gwen_and (
		.A(GWEN_reg0),
		.B(GWEN_reg1),
		.X(GWEN_reg)
	);

	gf180mcu_fd_ip_sram__sram128x8m8wm1 sram_inst (
		.CLK(CLK),
		.CEN(CEN_buf),
		.GWEN(GWEN_reg),
		.WEN(WEN),
		.A(A),
		.D(D),
		.Q(Q)
	);

endmodule
