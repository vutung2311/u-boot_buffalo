#include <common.h>
#include <config.h>
#include <asm/types.h>
#include <flash.h>

/*
 * sets up flash_info and returns size of FLASH (bytes)
 */
unsigned long flash_get_geom(flash_info_t *flash_info) {
    int i;

    /* XXX flash id is detected already */
    flash_info->size = CFG_FLASH_SIZE; /* bytes */
    flash_info->sector_count = flash_info->size / CFG_FLASH_SECTOR_SIZE;

    /* we use this special flash layout because we're making u-booto for whr-hp-g300n */
    for (i = 0; i < flash_info->sector_count; i++) {
        flash_info->start[i] = CFG_FLASH_BASE + i * CFG_FLASH_SECTOR_SIZE;
        flash_info->protect[i] = 0;
    }

    printf("flash size %d, sector count = %d\n", flash_info->size,
            flash_info->sector_count);
    return (flash_info->size);

}
