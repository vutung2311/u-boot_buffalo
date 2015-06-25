#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm-mips/regdef.h>
#include <config.h>
#include <version.h>
#include "ar7240_soc.h"

extern void ar7240_ddr_initial_config(uint32_t refresh);
extern int ar7240_ddr_find_size(void);

void ar7240_usb_initial_config(void) {
    ar7240_reg_wr_nf(AR7240_USB_PLL_CONFIG, 0x0a04081e);
    ar7240_reg_wr_nf(AR7240_USB_PLL_CONFIG, 0x0804081e);
}

void ar7240_gpio_config(void) {
    ar7240_reg_wr(AR7240_GPIO_FUNC,
            (ar7240_reg_rd(AR7240_GPIO_FUNC) & 0xEF84E0FB));

    /* Disable EJTAG functionality to enable GPIO functionality */
    ar7240_reg_wr(AR7240_GPIO_FUNC, (ar7240_reg_rd(AR7240_GPIO_FUNC) | 0x8001));

    /* Enable GPIO for WHR-HP-G300N */
    ar7240_reg_wr(AR7240_GPIO_OE, (ar7240_reg_rd(AR7240_GPIO_OE) | 0x3f9c3));
}

int ar7240_mem_config(void) {

    unsigned int tap_val1, tap_val2;
    ar7240_ddr_initial_config(CFG_DDR_REFRESH_VAL);

    /* Default tap values for starting the tap_init*/
    ar7240_reg_wr(AR7240_DDR_TAP_CONTROL0, 0x7);
    ar7240_reg_wr(AR7240_DDR_TAP_CONTROL1, 0x7);

    /* Called before ar7240_ddr_tap_init to fix the LED issues -- by lsz 09.03.25 */
    ar7240_usb_initial_config();
    ar7240_gpio_config();

    ar7240_ddr_tap_init();

    tap_val1 = ar7240_reg_rd(0xb800001c);
    tap_val2 = ar7240_reg_rd(0xb8000020);
    printf("#### TAP VALUE 1 = %x, 2 = %x\n", tap_val1, tap_val2);

    return (ar7240_ddr_find_size());
}

long int initdram(int board_type) {
    return (ar7240_mem_config());
}

int checkboard(void) {

    printf("AP93 (ar7240) U-boot\n");
    return 0;
}
