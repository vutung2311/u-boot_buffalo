#include <common.h>
#include <jffs2/jffs2.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include "ar7240_soc.h"
#include "ar7240_flash.h"

/*
 * globals
 */
flash_info_t flash_info[CFG_MAX_FLASH_BANKS];

#undef display
#define display(x)  ;
#define SIZE_INBYTES_4KBYTES	(4 * 1024)
#define SIZE_INBYTES_64KBYTES   (16 * SIZE_INBYTES_4KBYTES)
#define SIZE_INBYTES_4MBYTES    (4 * 1024 * 1024)
#define SIZE_INBYTES_8MBYTES	(2 * SIZE_INBYTES_4MBYTES)
#define SIZE_INBYTES_16MBYTES	(2 * SIZE_INBYTES_8MBYTES)

/*
 * statics
 */
static void
ar7240_spi_write_enable(void);
static void
ar7240_spi_poll(void);
static void
ar7240_spi_write_page(uint32_t addr, uint8_t *data, int len);
static void
ar7240_spi_sector_erase(uint32_t addr);

/*
 * Returns JEDEC ID from SPI flash
 */
static ulong read_id(void) {
    unsigned int flashid = 0;

    ar7240_reg_wr_nf(AR7240_SPI_FS, 1);
    ar7240_reg_wr_nf(AR7240_SPI_WRITE, AR7240_SPI_CS_DIS);

    ar7240_spi_bit_banger(0x9F);
    ar7240_spi_bit_banger(0);
    ar7240_spi_bit_banger(0);
    ar7240_spi_bit_banger(0);
    ar7240_spi_bit_banger(0);

    flashid = ar7240_reg_rd(AR7240_SPI_RD_STATUS);

    /*
     * We have 3 bytes:
     * - manufacture ID (1b)
     * - product ID (2b)
     */
    flashid = flashid >> 8;

    ar7240_spi_done();

    return ((ulong) flashid);
}

unsigned long flash_init(void) {

    ar7240_reg_wr_nf(AR7240_SPI_CLOCK, 0x43);

    // get flash id
    flash_info[0].flash_id = read_id();

    puts("FLASH:  ");

    // fill flash info based on JEDEC ID
    switch (flash_info[0].flash_id) {
        /*
         * 4M flash chips
         */
        case 0x010215:
            puts("Spansion S25FL032P (4 MB)");
            break;

        case 0x1F4700:
            puts("Atmel AT25DF321 (4 MB)");
            break;

        case 0x1C3016:
            puts("EON EN25Q32 (4 MB)");
            break;

        case 0x1C3116:
            puts("EON EN25F32 (4 MB)");
            break;

        case 0x202016:
            puts("Micron M25P32 (4 MB)");
            break;

        case 0xEF4016:
            puts("Winbond W25Q32 (4 MB)");
            break;

        case 0xC22016:
            puts("Macronix MX25L320 (4 MB)");
            break;

            /*
             * 8M flash chips
             */
        case 0x010216:
            puts("Spansion S25FL064P (8 MB)");
            break;

        case 0x1F4800:
            puts("Atmel AT25DF641 (8 MB)");
            break;

        case 0x1C3017:	// tested
            puts("EON EN25Q64 (8 MB)");
            break;

        case 0x202017:
            puts("Micron M25P64 (8 MB)");
            break;

        case 0xEF4017:	// tested
            puts("Winbond W25Q64 (8 MB)");
            break;

        case 0xC22017:
        case 0xC22617:
            puts("Macronix MX25L64 (8 MB)");
            break;

            /*
             * 16M flash chips
             */
        case 0xEF4018:	// tested
            puts("Winbond W25Q128 (16 MB)");
            break;

        case 0xC22018:
        case 0xC22618:
            puts("Macronix MX25L128 (16 MB)");
            break;

        case 0x012018:
            puts("Spansion S25FL127S (16 MB)");
            break;

        case 0x20BA18:
            puts("Micron N25Q128 (16 MB)");
            break;

            /*
             * Unknown flash
             */
        default:
#if (DEFAULT_FLASH_SIZE_IN_MB == 4)
            puts("Unknown type (using only 4 MB)\n");
#elif (DEFAULT_FLASH_SIZE_IN_MB == 8)
            puts("Unknown type (using only 8 MB)\n");
#elif (DEFAULT_FLASH_SIZE_IN_MB == 16)
            puts("Unknown type (using only 16 MB)\n");
#endif
            printf(
                    "\nPlease, send request to add support\nfor your FLASH - JEDEC ID: 0x%06lX\n",
                    flash_info[0].flash_id);
            flash_info[0].flash_id = FLASH_UNKNOWN;
            break;
    }

    puts("\n");

    return (flash_get_geom(&flash_info[0]));
}

void flash_print_info(flash_info_t *info) {
    printf("The hell do you want flinfo for??\n");
}

int flash_erase(flash_info_t *info, int s_first, int s_last) {
    int i, sector_size = info->size / info->sector_count;

    printf("\nFirst %#x last %#x sector size %#x\n", s_first, s_last,
            sector_size);

    for (i = s_first; i <= s_last; i++) {
        printf("\b\b\b\b%4d", i);
        ar7240_spi_sector_erase(i * sector_size);
    }
    ar7240_spi_done();
    printf("\n");

    return 0;
}

/*
 * Write a buffer from memory to flash:
 * 0. Assumption: Caller has already erased the appropriate sectors.
 * 1. call page programming for every 256 bytes
 */
int write_buff(flash_info_t *info, uchar *source, ulong addr, ulong len) {
    int total = 0, len_this_lp, bytes_this_page;
    ulong dst;
    uchar *src;

    printf("write addr: %x\n", addr);
    addr = addr - CFG_FLASH_BASE;

    while (total < len) {
        src = source + total;
        dst = addr + total;
        bytes_this_page = AR7240_SPI_PAGE_SIZE - (addr % AR7240_SPI_PAGE_SIZE);
        len_this_lp =
                ((len - total) > bytes_this_page) ?
                        bytes_this_page : (len - total);
        ar7240_spi_write_page(dst, src, len_this_lp);
        total += len_this_lp;
    }

    ar7240_spi_done();

    return 0;
}

static void ar7240_spi_write_enable() {
    ar7240_reg_wr_nf(AR7240_SPI_FS, 1);
    ar7240_reg_wr_nf(AR7240_SPI_WRITE, AR7240_SPI_CS_DIS);
    ar7240_spi_bit_banger(AR7240_SPI_CMD_WREN);
    ar7240_spi_go()
    ;
}

static void ar7240_spi_poll() {
    int rd;

    do {
        ar7240_reg_wr_nf(AR7240_SPI_WRITE, AR7240_SPI_CS_DIS);
        ar7240_spi_bit_banger(AR7240_SPI_CMD_RD_STATUS);
        ar7240_spi_delay_8()
        ;
        rd = (ar7240_reg_rd(AR7240_SPI_RD_STATUS) & 1);
    } while (rd);
}

static void ar7240_spi_write_page(uint32_t addr, uint8_t *data, int len) {
    int i;
    uint8_t ch;

    display(0x77);
    ar7240_spi_write_enable();
    ar7240_spi_bit_banger(AR7240_SPI_CMD_PAGE_PROG);
    ar7240_spi_send_addr(addr);

    for (i = 0; i < len; i++) {
        ch = *(data + i);
        ar7240_spi_bit_banger(ch);
    }

    ar7240_spi_go()
    ;display(0x66);
    ar7240_spi_poll();
    display(0x6d);
}

static void ar7240_spi_sector_erase(uint32_t addr) {
    ar7240_spi_write_enable();
    ar7240_spi_bit_banger(AR7240_SPI_CMD_SECTOR_ERASE);
    ar7240_spi_send_addr(addr);
    ar7240_spi_go()
    ;display(0x7d);
    ar7240_spi_poll();
}

