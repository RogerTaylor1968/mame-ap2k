/*****************************************************************************
 *
 * includes/partner.h
 *
 ****************************************************************************/

#ifndef partner_H_
#define partner_H_

#include "machine/i8255.h"
#include "machine/8257dma.h"
#include "machine/wd17xx.h"
#include "machine/ram.h"

class partner_state : public radio86_state
{
public:
	partner_state(const machine_config &mconfig, device_type type, const char *tag)
		: radio86_state(mconfig, type, tag),
		m_ram(*this, RAM_TAG) { }

	UINT8 m_mem_page;
	UINT8 m_win_mem_page;
	DECLARE_READ8_MEMBER(partner_floppy_r);
	DECLARE_WRITE8_MEMBER(partner_floppy_w);
	DECLARE_WRITE8_MEMBER(partner_win_memory_page_w);
	DECLARE_WRITE8_MEMBER(partner_mem_page_w);
	DECLARE_DRIVER_INIT(partner);
	DECLARE_MACHINE_START(partner);
	DECLARE_MACHINE_RESET(partner);
	DECLARE_WRITE_LINE_MEMBER(partner_wd17xx_drq_w);
	DECLARE_WRITE_LINE_MEMBER(hrq_w);
	void partner_window_1(UINT8 bank_num, UINT16 offset,UINT8 *rom);
	void partner_window_2(UINT8 bank_num, UINT16 offset,UINT8 *rom);
	void partner_iomap_bank(UINT8 *rom);
	void partner_bank_switch();
	required_device<ram_device> m_ram;
};


/*----------- defined in machine/partner.c -----------*/

extern const i8257_interface partner_dma;
extern const wd17xx_interface partner_wd17xx_interface;

#endif /* partner_H_ */
