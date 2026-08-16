#include "ata.h"
#include "kernel.h"
#include "libc.h"

#define ATA_DATA_PORT         0x1F0
#define ATA_FEATURES_PORT     0x1F1
#define ATA_SECTOR_COUNT_PORT 0x1F2
#define ATA_LBA_LOW_PORT      0x1F3
#define ATA_LBA_MID_PORT      0x1F4
#define ATA_LBA_HIGH_PORT     0x1F5
#define ATA_DRIVE_PORT        0x1F6
#define ATA_COMMAND_PORT      0x1F7
#define ATA_STATUS_PORT       0x1F7

#define ATA_CMD_READ_PIO      0x20
#define ATA_CMD_WRITE_PIO     0x30

static void ata_wait_bsy(void) {
    while (inb(ATA_STATUS_PORT) & 0x80);
}

static void ata_wait_drq(void) {
    while (!(inb(ATA_STATUS_PORT) & 0x08));
}

void ata_init(void) {
    // Basic init or identify can be done here.
}

void ata_read_sector(uint32_t lba, uint8_t *buffer) {
    ata_wait_bsy();
    outb(ATA_DRIVE_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT_PORT, 1);
    outb(ATA_LBA_LOW_PORT, (uint8_t)lba);
    outb(ATA_LBA_MID_PORT, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH_PORT, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND_PORT, ATA_CMD_READ_PIO);
    
    ata_wait_bsy();
    ata_wait_drq();
    
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_DATA_PORT);
        buffer[i * 2] = (uint8_t)(data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t)((data >> 8) & 0xFF);
    }
}

void ata_write_sector(uint32_t lba, const uint8_t *buffer) {
    ata_wait_bsy();
    outb(ATA_DRIVE_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_COUNT_PORT, 1);
    outb(ATA_LBA_LOW_PORT, (uint8_t)lba);
    outb(ATA_LBA_MID_PORT, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH_PORT, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND_PORT, ATA_CMD_WRITE_PIO);
    
    ata_wait_bsy();
    ata_wait_drq();
    
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        outw(ATA_DATA_PORT, data);
    }
    
    // Flush cache
    outb(ATA_COMMAND_PORT, 0xEA);
    ata_wait_bsy();
}
