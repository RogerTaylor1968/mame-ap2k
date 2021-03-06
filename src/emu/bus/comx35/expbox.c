// license:BSD-3-Clause
// copyright-holders:Curt Coder
/**********************************************************************

    COMX-35E Expansion Box emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

(c) 1984 Comx World Operations

PCB Layout
----------

F-001-EB-REV0

     |--------------------------------------|
     |  40174       4073    4049    4075    |
     |                                      |
     |  ROM         40175   4073    4075    |
     |                                      |
|----|      -       -       -       -       |
|           |       |       |       | 7805  |
|           |       |       |       |       |
|           |       |       |       |       |
|C          C       C       C       C       |
|N          N       N       N       N       |
|5          1       2       3       4       |
|           |       |       |       |       |
|           |       |       |       |       |
|           |       |       |       |       |
|           -       -       - LD1   -       |
|-------------------------------------------|

Notes:
    All IC's shown.

    ROM     - NEC D2732D-4 4Kx8 EPROM, unlabeled
    CN1     - COMX-35 bus connector slot 1
    CN2     - COMX-35 bus connector slot 2
    CN3     - COMX-35 bus connector slot 3
    CN4     - COMX-35 bus connector slot 4
    CN5     - COMX-35 bus PCB edge connector
    LD1     - LED

*/

#include "expbox.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define SLOT1_TAG           "slot1"
#define SLOT2_TAG           "slot2"
#define SLOT3_TAG           "slot3"
#define SLOT4_TAG           "slot4"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type COMX_EB = &device_creator<comx_eb_device>;


//-------------------------------------------------
//  ROM( comx_eb )
//-------------------------------------------------

ROM_START( comx_eb )
	ROM_REGION( 0x1000, "e000", 0 )
	ROM_SYSTEM_BIOS( 0, "comx", "Original" )
	ROMX_LOAD( "expansion.e5",         0x0000, 0x1000, CRC(52cb44e2) SHA1(3f9a3d9940b36d4fee5eca9f1359c99d7ed545b9), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "fm31", "F&M 3.1" )
	ROMX_LOAD( "f&m.expansion.3.1.e5", 0x0000, 0x1000, CRC(818ca2ef) SHA1(ea000097622e7fd472d53e7899e3c83773433045), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "fm32", "F&M 3.2" )
	ROMX_LOAD( "f&m.expansion.3.2.e5", 0x0000, 0x1000, CRC(0f0fc960) SHA1(eb6b6e7bc9e761d13554482025d8cb5e260c0619), ROM_BIOS(3) )
ROM_END


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *comx_eb_device::device_rom_region() const
{
	return ROM_NAME( comx_eb );
}


//-------------------------------------------------
//  COMX_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

WRITE_LINE_DEVICE_HANDLER( int_w )
{
	comx_eb_device *eb = downcast<comx_eb_device *>(device->owner());
	eb->set_int(device->tag(), state);
}

WRITE_LINE_DEVICE_HANDLER( wait_w )
{
	comx_expansion_slot_device *slot = dynamic_cast<comx_expansion_slot_device *>(device->owner()->owner());
	slot->wait_w(state);
}

WRITE_LINE_DEVICE_HANDLER( clear_w )
{
	comx_expansion_slot_device *slot = dynamic_cast<comx_expansion_slot_device *>(device->owner()->owner());
	slot->clear_w(state);
}

static COMX_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_LINE(int_w),
	DEVCB_LINE(wait_w),
	DEVCB_LINE(clear_w)
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( comx_eb )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( comx_eb )
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT1_TAG, expansion_intf, comx_expansion_cards, "fd")
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT2_TAG, expansion_intf, comx_expansion_cards, "clm")
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT3_TAG, expansion_intf, comx_expansion_cards, "joy")
	MCFG_COMX_EXPANSION_SLOT_ADD(SLOT4_TAG, expansion_intf, comx_expansion_cards, "ram")
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor comx_eb_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( comx_eb );
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  set_int - set INT line state
//-------------------------------------------------

void comx_eb_device::set_int(const char *tag, int state)
{
	int slot = 0;

	for (slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (!strcmp(tag, m_expansion_slot[slot]->tag())) break;
	}

	assert(slot < MAX_EB_SLOTS);

	m_int[slot] = state;

	int irq = CLEAR_LINE;

	for (slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		irq |= m_int[slot];
	}

	m_slot->int_w(irq);
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  comx_eb_device - constructor
//-------------------------------------------------

comx_eb_device::comx_eb_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, COMX_EB, "COMX-35E Expansion Box", tag, owner, clock, "comx_eb", __FILE__),
	device_comx_expansion_card_interface(mconfig, *this),
	m_rom(*this, "e000"),
	m_select(0)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void comx_eb_device::device_start()
{
	m_expansion_slot[0] = dynamic_cast<comx_expansion_slot_device *>(subdevice(SLOT1_TAG));
	m_expansion_slot[1] = dynamic_cast<comx_expansion_slot_device *>(subdevice(SLOT2_TAG));
	m_expansion_slot[2] = dynamic_cast<comx_expansion_slot_device *>(subdevice(SLOT3_TAG));
	m_expansion_slot[3] = dynamic_cast<comx_expansion_slot_device *>(subdevice(SLOT4_TAG));

	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		m_int[slot] = CLEAR_LINE;
	}
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void comx_eb_device::device_reset()
{
	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (m_expansion_slot[slot] != NULL)
		{
			m_expansion_slot[slot]->device().reset();
			m_expansion_slot[slot]->ds_w(0);
		}
	}
}


//-------------------------------------------------
//  comx_ef4_r - external flag 4 read
//-------------------------------------------------

int comx_eb_device::comx_ef4_r()
{
	int state = CLEAR_LINE;

	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (m_expansion_slot[slot] != NULL)
		{
			state |= m_expansion_slot[slot]->ef4_r();
		}
	}

	return state;
}


//-------------------------------------------------
//  comx_q_w - Q write
//-------------------------------------------------

void comx_eb_device::comx_q_w(int state)
{
	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (m_expansion_slot[slot] != NULL)
		{
			m_expansion_slot[slot]->q_w(state);
		}
	}
}


//-------------------------------------------------
//  comx_mrd_r - memory read
//-------------------------------------------------

UINT8 comx_eb_device::comx_mrd_r(address_space &space, offs_t offset, int *extrom)
{
	UINT8 data = 0;

	if (offset >= 0x1000 && offset < 0x1800)
	{
		data = m_rom->base()[offset & 0x7ff];
		*extrom = 0;
	}
	else if (offset >= 0xe000 && offset < 0xf000)
	{
		data = m_rom->base()[offset & 0xfff];
	}
	else
	{
		for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
		{
			if (BIT(m_select, slot) && m_expansion_slot[slot] != NULL)
			{
				data |= m_expansion_slot[slot]->mrd_r(space, offset, extrom);
			}
		}
	}

	return data;
}


//-------------------------------------------------
//  comx_mwr_w - memory write
//-------------------------------------------------

void comx_eb_device::comx_mwr_w(address_space &space, offs_t offset, UINT8 data)
{
	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (BIT(m_select, slot) && m_expansion_slot[slot] != NULL)
		{
			m_expansion_slot[slot]->mwr_w(space, offset, data);
		}
	}
}


//-------------------------------------------------
//  comx_io_r - I/O read
//-------------------------------------------------

UINT8 comx_eb_device::comx_io_r(address_space &space, offs_t offset)
{
	UINT8 data = 0;

	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (BIT(m_select, slot) && m_expansion_slot[slot] != NULL)
		{
			data |= m_expansion_slot[slot]->io_r(space, offset);
		}
	}

	return data;
}


//-------------------------------------------------
//  comx_io_w - I/O write
//-------------------------------------------------

void comx_eb_device::comx_io_w(address_space &space, offs_t offset, UINT8 data)
{
	if (offset == 1 && !(BIT(data, 0)))
	{
		m_select = data >> 1;

		for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
		{
			if (m_expansion_slot[slot] != NULL)
			{
				m_expansion_slot[slot]->ds_w(BIT(m_select, slot));
			}
		}
	}

	for (int slot = 0; slot < MAX_EB_SLOTS; slot++)
	{
		if (BIT(m_select, slot) && m_expansion_slot[slot] != NULL)
		{
			m_expansion_slot[slot]->io_w(space, offset, data);
		}
	}
}
