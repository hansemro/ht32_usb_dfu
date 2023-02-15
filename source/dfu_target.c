#include "usbdfu.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

size_t target_get_max_fw_size(void) {
	return (MAX_FLASH_ADDR - APP_BASE);
}

uint16_t target_get_timeout(void) {
    return 5;
}

void target_flash_unlock(void) {
	chSysLock();
}

static bool ht32_program_word(uint32_t addr, uint32_t value) {
    FMC->TADR = addr;
    FMC->WRDR = value;
    FMC->OCMR = FMC_OCMR_CMD_WORD_PROGRAM;
    FMC->OPCR = FMC_OPCR_OPM_COMMIT;

    // wait until finished
    int timeout = 0xf000;
    for (;;) {
        if ((FMC->OPCR & FMC_OPCR_OPM_MASK) == FMC_OPCR_OPM_FINISHED)
            break;
        if (timeout-- <= 0)
            return false;
    }
    return true;
}

bool target_flash_write(uint8_t* dst, uint8_t* src, size_t len) {
    while (len > 4) {
        if (!ht32_program_word((uint32_t)dst, *(uint32_t*)src))
            return false;
        dst += 4;
        src += 4;
        len -= 4;
    }
    if (len > 0) {
        uint32_t buf = 0;
        memcpy(&buf, src, len);
        if (!ht32_program_word((uint32_t)dst, buf))
            return false;
    }
    return true;
}

static bool ht32_erase_page(uint32_t page_addr) {
    // align to page size
    page_addr &= ~(FLASH_PAGE_SIZE - 1);

    FMC->TADR = page_addr;
    FMC->OCMR = FMC_OCMR_CMD_PAGE_ERASE;
    FMC->OPCR = FMC_OPCR_OPM_COMMIT;

    // wait until finished
    int timeout = 0x1000;
    for (;;) {
        if ((FMC->OPCR & FMC_OPCR_OPM_MASK) == FMC_OPCR_OPM_FINISHED)
            break;
        if (timeout-- <= 0)
            return false;
    }
    return true;
}

bool target_prepare_flash(void) {
    // Erase App Flash Pages
    for (uint32_t page_addr = APP_BASE; page_addr < MAX_FLASH_ADDR; page_addr += FLASH_PAGE_SIZE) {
        if (!ht32_erase_page(page_addr))
            return false;
    }
    return true;
}

void target_flash_lock(void) {
	chSysUnlock();
}

void target_complete_programming(void) {
    // Do nothing
}
