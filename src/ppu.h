#ifndef YOBEMAG_PPU_H
#define YOBEMAG_PPU_H

#include <stdint.h>

#define VISIBLE_WIDTH  160
#define VISIBLE_HEIGHT 144

#define PPU_REG_LCDC 0xFF40
#define PPU_REG_STAT 0xFF41
#define PPU_REG_SCY  0xFF42
#define PPU_REG_SCX  0xFF43
#define PPU_REG_LY   0xFF44
#define PPU_REG_LYC  0xFF45
#define PPU_REG_DMA  0xFF46
#define PPU_REG_BGP  0xFF47
#define PPU_REG_OBP0 0xFF48
#define PPU_REG_OBP1 0xFF49
#define PPU_REG_WY   0xFF4A
#define PPU_REG_WX   0xFF4B

void ppu_init(void);

#endif // YOBEMAG_PPU_H
