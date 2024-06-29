all: stub rtlsim simx opae

stub:
	$(MAKE) -C stub

simx:
	$(MAKE) -C simx

rtlsim:
	$(MAKE) -C rtlsim

opae:
	$(MAKE) -C opae

xrt:
	$(MAKE) -C xrt

som:
	$(MAKE) -C som

clean:
	$(MAKE) clean -C stub
	$(MAKE) clean -C simx
	$(MAKE) clean -C rtlsim
	$(MAKE) clean -C opae
	$(MAKE) clean -C xrt
	$(MAKE) clean -C som

.PHONY: all stub simx rtlsim opae xrt som clean