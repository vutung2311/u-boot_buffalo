# Export variables
export BUILD_TOPDIR=$(PWD)
export STAGING_DIR=$(BUILD_TOPDIR)/tmp
export TFTPPATH=$(HOME)
export CROSS_COMPILE=/home/tung/opt/mips-gcc-openwrt/bin/mips-openwrt-linux-
export MAKECMD=make --silent --no-print-directory ARCH=mips CROSS_COMPILE=$(CROSS_COMPILE)

whr_hp_g300n:
# Build u-boot for real
	@cd $(BUILD_TOPDIR)/u-boot/ && $(MAKECMD) ap93_config
	@cd $(BUILD_TOPDIR)/u-boot/ && $(MAKECMD) ENDIANNESS=-EB V=1 all
	@cp $(BUILD_TOPDIR)/u-boot/u-boot.bin $(HOME)/
	
clean:
	@rm -rf $(HOME)/u-boot.bin
	@cd $(BUILD_TOPDIR)/u-boot/ && $(MAKECMD) --no-print-directory distclean