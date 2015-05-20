/*                                                                      */
/* eeprom.c                                                             */
/*                                                                      */
/* Alex Forencich <alex@alexforencich.com>                              */
/*                                                                      */
/* Copyright (c) 2011 Alex Forencich                                    */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation       */
/* files(the "Software"), to deal in the Software without restriction,  */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies or substantial portions of the Software.      */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS  */
/* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN   */
/* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN    */
/* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     */
/* SOFTWARE.                                                            */
/*                                                                      */
/************************************************************************/

#include "eeprom.h"
#include "string.h"

// NVM call
static inline void NVM_EXEC(void)
{
        void *z = (void *)&NVM_CTRLA;
        
        __asm__ volatile("out %[ccp], %[ioreg]"  "\n\t"
        "st z, %[cmdex]"
        :
        : [ccp] "I" (_SFR_IO_ADDR(CCP)),
        [ioreg] "d" (CCP_IOREG_gc),
                     [cmdex] "r" (NVM_CMDEX_bm),
                     [z] "z" (z)
                     );
}

void wait_for_nvm(void)
{
        while (NVM.STATUS & NVM_NVMBUSY_bm) { };
}

uint8_t EEPROM_read_byte(uint16_t addr)
{
        wait_for_nvm();
        
        NVM.ADDR0 = addr & 0xFF;
        NVM.ADDR1 = (addr >> 8) & 0x1F;
        NVM.ADDR2 = 0;
        
        NVM.CMD = NVM_CMD_READ_EEPROM_gc;
        NVM_EXEC();
        
        return NVM.DATA0;
}


uint16_t EEPROM_read_block(uint16_t addr, uint8_t *dest, uint16_t len)
{
        uint16_t cnt = 0;
        
        NVM.ADDR2 = 0;
        
        wait_for_nvm();
        
        while (len > 0)
        {
                NVM.ADDR0 = addr & 0xFF;
                NVM.ADDR1 = (addr >> 8) & 0x1F;
                
                NVM.CMD = NVM_CMD_READ_EEPROM_gc;
                NVM_EXEC();
                
                *(dest++) = NVM.DATA0; addr++;
                
                len--; cnt++;
        }
        return cnt;
}
