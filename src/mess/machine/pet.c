/***************************************************************************
    commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"

#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"

#include "includes/pet.h"

#include "imagedev/cartslot.h"
#include "machine/ram.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG( MACHINE, N, M, A ) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", MACHINE.time().as_double(), (char*) M ); \
			logerror A; \
		} \
	} while (0)



/* pia at 0xe810
   port a
    7 sense input (low for diagnostics)
    6 ieee eoi in
    5 cassette 2 switch in
    4 cassette 1 switch in
    3..0 keyboard line select

  ca1 cassette 1 read
  ca2 ieee eoi out

  cb1 video sync in
  cb2 cassette 1 motor out
*/
READ8_MEMBER(pet_state::pia0_pa_r)
{
	/*

	    bit     description

	    PA0     KEY A
	    PA1     KEY B
	    PA2     KEY C
	    PA3     KEY D
	    PA4     #1 CASS SWITCH
	    PA5     #2 CASS SWITCH
	    PA6     _EOI IN
	    PA7     DIAG JUMPER

	*/

	UINT8 data = 0;

	/* key */
	data |= m_keyline_select;

	/* #1 cassette switch */
	data |= m_cassette->sense_r() << 4;

	/* #2 cassette switch */
	data |= m_cassette2->sense_r() << 5;

	/* end or identify in */
	data |= m_ieee->eoi_r() << 6;

	/* diagnostic jumper */
	data |= 0x80;

	return data;
}

WRITE8_MEMBER(pet_state::pia0_pa_w)
{
	/*

	    bit     description

	    PA0     KEY A
	    PA1     KEY B
	    PA2     KEY C
	    PA3     KEY D
	    PA4     #1 CASS SWITCH
	    PA5     #2 CASS SWITCH
	    PA6     _EOI IN
	    PA7     DIAG JUMPER

	*/

	/* key */
	m_keyline_select = data & 0x0f;
}

/* Keyboard reading/handling for regular keyboard */
READ8_MEMBER(pet_state::kin_r)
{
	/*

	    bit     description

	    PB0     KIN0
	    PB1     KIN1
	    PB2     KIN2
	    PB3     KIN3
	    PB4     KIN4
	    PB5     KIN5
	    PB6     KIN6
	    PB7     KIN7

	*/

	UINT8 data = 0xff;
	static const char *const keynames[] = {
		"ROW0", "ROW1", "ROW2", "ROW3", "ROW4",
		"ROW5", "ROW6", "ROW7", "ROW8", "ROW9"
	};

	if (m_keyline_select < 10)
	{
		data = machine().root_device().ioport(keynames[m_keyline_select])->read();
		/* Check for left-shift lock */
		if ((m_keyline_select == 8) && (machine().root_device().ioport("SPECIAL")->read() & 0x80))
			data &= 0xfe;
	}
	return data;
}

/* Keyboard handling for business keyboard */
READ8_MEMBER(pet_state::petb_kin_r)
{
	UINT8 data = 0xff;
	static const char *const keynames[] = {
		"ROW0", "ROW1", "ROW2", "ROW3", "ROW4",
		"ROW5", "ROW6", "ROW7", "ROW8", "ROW9"
	};

	if (m_keyline_select < 10)
	{
		data = machine().root_device().ioport(keynames[m_keyline_select])->read();
		/* Check for left-shift lock */
		/* 2008-05 FP: For some reason, superpet read it in the opposite way!! */
		/* While waiting for confirmation from docs, we add a workaround here. */
		if (m_superpet)
		{
			if ((m_keyline_select == 6) && !(machine().root_device().ioport("SPECIAL")->read() & 0x80))
				data &= 0xfe;
		}
		else
		{
			if ((m_keyline_select == 6) && (machine().root_device().ioport("SPECIAL")->read() & 0x80))
				data &= 0xfe;
		}
	}
	return data;
}

READ8_MEMBER(pet_state::cass1_r)
{
	// cassette 1 read
	return m_cassette->read();
}

WRITE8_MEMBER(pet_state::cass1_motor_w)
{
	m_cassette->motor_w(data);
}

WRITE_LINE_MEMBER(pet_state::pia0_irq_w)
{
	m_pia0_irq = state;
	int level = (m_pia0_irq | m_pia1_irq | m_via_irq) ? ASSERT_LINE : CLEAR_LINE;

	machine().firstcpu->set_input_line(INPUT_LINE_IRQ0, level);
}

const pia6821_interface pet_pia0 =
{
	DEVCB_DRIVER_MEMBER(pet_state,pia0_pa_r),       /* in_a_func */
	DEVCB_DRIVER_MEMBER(pet_state,kin_r),           /* in_b_func */
	DEVCB_DRIVER_MEMBER(pet_state,cass1_r),         /* in_ca1_func */
	DEVCB_NULL,                     /* in_cb1_func */
	DEVCB_NULL,                     /* in_ca2_func */
	DEVCB_NULL,                     /* in_cb2_func */
	DEVCB_DRIVER_MEMBER(pet_state,pia0_pa_w),       /* out_a_func */
	DEVCB_NULL,                     /* out_b_func */
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, eoi_w),           /* out_ca2_func */
	DEVCB_DRIVER_MEMBER(pet_state,cass1_motor_w),   /* out_cb2_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,pia0_irq_w),         /* irq_a_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,pia0_irq_w)          /* irq_b_func */
};

const pia6821_interface petb_pia0 =
{
	DEVCB_DRIVER_MEMBER(pet_state,pia0_pa_r),       /* in_a_func */
	DEVCB_DRIVER_MEMBER(pet_state,petb_kin_r),      /* in_b_func */
	DEVCB_DRIVER_MEMBER(pet_state,cass1_r),         /* in_ca1_func */
	DEVCB_NULL,                     /* in_cb1_func */
	DEVCB_NULL,                     /* in_ca2_func */
	DEVCB_NULL,                     /* in_cb2_func */
	DEVCB_DRIVER_MEMBER(pet_state,pia0_pa_w),       /* out_a_func */
	DEVCB_NULL,                     /* out_b_func */
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, eoi_w),           /* out_ca2_func */
	DEVCB_DRIVER_MEMBER(pet_state,cass1_motor_w),   /* out_cb2_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,pia0_irq_w),         /* irq_a_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,pia0_irq_w)          /* irq_b_func */
};

/* pia at 0xe820 (ieee488)
   port a data in
   port b data out
  ca1 atn in
  ca2 ndac out
  cb1 srq in
  cb2 dav out
 */

WRITE_LINE_MEMBER(pet_state::pia1_irq_w)
{
	m_pia1_irq = state;
	int level = (m_pia0_irq || m_pia1_irq || m_via_irq) ? ASSERT_LINE : CLEAR_LINE;

	machine().firstcpu->set_input_line(INPUT_LINE_IRQ0, level);
}

const pia6821_interface pet_pia1 =
{
	DEVCB_DEVICE_MEMBER(IEEE488_TAG, ieee488_device, dio_r),/* in_a_func */
	DEVCB_NULL,                             /* in_b_func */
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, atn_r),   /* in_ca1_func */
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, srq_r),   /* in_cb1_func */
	DEVCB_NULL,                             /* in_ca2_func */
	DEVCB_NULL,                             /* in_cb2_func */
	DEVCB_NULL,                             /* out_a_func */
	DEVCB_DEVICE_MEMBER(IEEE488_TAG, ieee488_device, dio_w),                    /* out_b_func */
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, ndac_w),                  /* out_ca2_func */
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, dav_w),                   /* out_cb2_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,pia1_irq_w),                 /* irq_a_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,pia1_irq_w)                  /* irq_b_func */
};

/* userport, cassettes, rest ieee488
   ca1 userport
   ca2 character rom address line 11
   pa user port

   pb0 ieee ndac in
   pb1 ieee nrfd out
   pb2 ieee atn out
   pb3 userport/cassettes
   pb4 cassettes
   pb5 userport/???
   pb6 ieee nrfd in
   pb7 ieee dav in

   cb1 cassettes
   cb2 user port
 */
READ8_MEMBER(pet_state::via_pb_r)
{
	/*

	    bit     description

	    PB0     _NDAC IN
	    PB1     _NRFD OUT
	    PB2     _ATN OUT
	    PB3     CASS WRITE
	    PB4     #2 CASS MOTOR
	    PB5     SYNC IN
	    PB6     _NRFD IN
	    PB7     _DAV IN

	*/

	UINT8 data = 0;

	/* not data accepted in */
	data |= m_ieee->ndac_r();

	/* sync in */

	/* not ready for data in */
	data |= m_ieee->nrfd_r() << 6;

	/* data valid in */
	data |= m_ieee->dav_r() << 7;

	return data;
}

READ_LINE_MEMBER(pet_state::cass2_r)
{
	return m_cassette2->read();
}

WRITE8_MEMBER(pet_state::via_pb_w)
{
	/*

	    bit     description

	    PB0     _NDAC IN
	    PB1     _NRFD OUT
	    PB2     _ATN OUT
	    PB3     CASS WRITE
	    PB4     #2 CASS MOTOR
	    PB5     SYNC IN
	    PB6     _NRFD IN
	    PB7     _DAV IN

	*/

	/* not ready for data out */
	m_ieee->nrfd_w(BIT(data, 1));

	/* attention out */
	m_ieee->atn_w(BIT(data, 2));

	/* cassette write */
	m_cassette->write(BIT(data, 3));
	m_cassette2->write(BIT(data, 3));

	/* #2 cassette motor */
	m_cassette2->motor_w(BIT(data, 4));
}

WRITE_LINE_MEMBER(pet_state::gb_w)
{
	DBG_LOG(machine(), 1, "address line", ("%d\n", state));
	if (state) m_font |= 1;
	else m_font &= ~1;
}

WRITE_LINE_MEMBER(pet_state::via_irq_w)
{
	m_via_irq = state;
	int level = (m_pia0_irq | m_pia1_irq | m_via_irq) ? ASSERT_LINE : CLEAR_LINE;

	machine().firstcpu->set_input_line(INPUT_LINE_IRQ0, level);
}

const via6522_interface pet_via =
{
	DEVCB_NULL,                 /* in_a_func */
	DEVCB_DRIVER_MEMBER(pet_state,via_pb_r),    /* in_b_func */
	DEVCB_NULL,                 /* in_ca1_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,cass2_r),        /* in_cb1_func */
	DEVCB_NULL,                 /* in_ca2_func */
	DEVCB_NULL,                 /* in_cb2_func */
	DEVCB_NULL,                 /* out_a_func */
	DEVCB_DRIVER_MEMBER(pet_state,via_pb_w),    /* out_b_func */
	DEVCB_NULL,                 /* out_ca1_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,gb_w),           /* out_ca2_func */
	DEVCB_NULL,                 /* out_ca2_func */
	DEVCB_NULL,                 /* out_cb2_func */
	DEVCB_DRIVER_LINE_MEMBER(pet_state,via_irq_w)       /* out_irq_func */
};


static WRITE8_HANDLER( cbm8096_io_w )
{
	via6522_device *via_0 = space.machine().device<via6522_device>("via6522_0");
	pia6821_device *pia_0 = space.machine().device<pia6821_device>("pia_0");
	pia6821_device *pia_1 = space.machine().device<pia6821_device>("pia_1");
	mc6845_device *mc6845 = space.machine().device<mc6845_device>("crtc");

	if (offset < 0x10) ;
	else if (offset < 0x14) pia_0->write(space, offset & 3, data);
	else if (offset < 0x20) ;
	else if (offset < 0x24) pia_1->write(space, offset & 3, data);
	else if (offset < 0x40) ;
	else if (offset < 0x50) via_0->write(space, offset & 0xf, data);
	else if (offset < 0x80) ;
	else if (offset == 0x80) mc6845->address_w(space, 0, data);
	else if (offset == 0x81) mc6845->register_w(space, 0, data);
}

static READ8_HANDLER( cbm8096_io_r )
{
	via6522_device *via_0 = space.machine().device<via6522_device>("via6522_0");
	pia6821_device *pia_0 = space.machine().device<pia6821_device>("pia_0");
	pia6821_device *pia_1 = space.machine().device<pia6821_device>("pia_1");
	mc6845_device *mc6845 = space.machine().device<mc6845_device>("crtc");

	int data = 0xff;
	if (offset < 0x10) ;
	else if (offset < 0x14) data = pia_0->read(space, offset & 3);
	else if (offset < 0x20) ;
	else if (offset < 0x24) data = pia_1->read(space, offset & 3);
	else if (offset < 0x40) ;
	else if (offset < 0x50) data = via_0->read(space, offset & 0xf);
	else if (offset < 0x80) ;
	else if (offset == 0x81) data = mc6845->register_r(space, 0);
	return data;
}

static WRITE8_HANDLER( pet80_bank1_w )
{
	pet_state *state = space.machine().driver_data<pet_state>();
	state->m_pet80_bank1_base[offset] = data;
}

/*
65520        8096 memory control register
        bit 0    1=write protect $8000-BFFF
        bit 1    1=write protect $C000-FFFF
        bit 2    $8000-BFFF bank select
        bit 3    $C000-FFFF bank select
        bit 5    1=screen peek through
        bit 6    1=I/O peek through
        bit 7    1=enable expansion memory

*/
WRITE8_HANDLER( cbm8096_w )
{
	pet_state *state = space.machine().driver_data<pet_state>();
	if (data & 0x80)
	{
		if (data & 0x40)
		{
			space.install_legacy_read_handler(0xe800, 0xefff, FUNC(cbm8096_io_r));
			space.install_legacy_write_handler(0xe800, 0xefff, FUNC(cbm8096_io_w));
		}
		else
		{
			space.install_read_bank(0xe800, 0xefff, "bank7");
			if (!(data & 2))
				space.install_write_bank(0xe800, 0xefff, "bank7");
			else
				space.nop_write(0xe800, 0xefff);
		}


		if ((data & 2) == 0) {
			space.install_write_bank(0xc000, 0xe7ff, "bank6");
			space.install_write_bank(0xf000, 0xffef, "bank8");
			space.install_write_bank(0xfff1, 0xffff, "bank9");
		} else {
			space.nop_write(0xc000, 0xe7ff);
			space.nop_write(0xf000, 0xffef);
			space.nop_write(0xfff1, 0xffff);
		}

		if (data & 0x20)
		{
			state->m_pet80_bank1_base = state->m_memory + 0x8000;
			state->membank("bank1")->set_base(state->m_pet80_bank1_base);
			space.install_legacy_write_handler(0x8000, 0x8fff, FUNC(pet80_bank1_w));
		}
		else
		{
			if (!(data & 1))
				space.install_write_bank(0x8000, 0x8fff, "bank1");
			else
				space.nop_write(0x8000, 0x8fff);
		}

		if ((data & 1) == 0 ){
			space.install_write_bank(0x9000, 0x9fff, "bank2");
			space.install_write_bank(0xa000, 0xafff, "bank3");
			space.install_write_bank(0xb000, 0xbfff, "bank4");
		} else {
			space.nop_write(0x9000, 0x9fff);
			space.nop_write(0xa000, 0xafff);
			space.nop_write(0xb000, 0xbfff);
		}

		if (data & 4)
		{
			if (!(data & 0x20))
			{
				state->m_pet80_bank1_base = state->m_memory + 0x14000;
				state->membank("bank1")->set_base(state->m_pet80_bank1_base);
			}
			state->membank("bank2")->set_base(state->m_memory + 0x15000);
			state->membank("bank3")->set_base(state->m_memory + 0x16000);
			state->membank("bank4")->set_base(state->m_memory + 0x17000);
		}
		else
		{
			if (!(data & 0x20))
			{
				state->m_pet80_bank1_base = state->m_memory + 0x10000;
				state->membank("bank1")->set_base(state->m_pet80_bank1_base);
			}
			state->membank("bank2")->set_base(state->m_memory + 0x11000);
			state->membank("bank3")->set_base(state->m_memory + 0x12000);
			state->membank("bank4")->set_base(state->m_memory + 0x13000);
		}

		if (data & 8)
		{
			if (!(data & 0x40))
			{
				state->membank("bank7")->set_base(state->m_memory + 0x1e800);
			}
			state->membank("bank6")->set_base(state->m_memory + 0x1c000);
			state->membank("bank8")->set_base(state->m_memory + 0x1f000);
			state->membank("bank9")->set_base(state->m_memory + 0x1fff1);
		}
		else
		{
			if (!(data & 0x40))
			{
				state->membank("bank7")->set_base(state->m_memory+ 0x1a800);
			}
			state->membank("bank6")->set_base(state->m_memory + 0x18000);
			state->membank("bank8")->set_base(state->m_memory + 0x1b000);
			state->membank("bank9")->set_base(state->m_memory + 0x1bff1);
		}
	}
	else
	{
		state->m_pet80_bank1_base = state->m_memory + 0x8000;
		state->membank("bank1")->set_base(state->m_pet80_bank1_base );
		space.install_legacy_write_handler(0x8000, 0x8fff, FUNC(pet80_bank1_w));

		state->membank("bank2")->set_base(state->m_memory + 0x9000);
		space.unmap_write(0x9000, 0x9fff);

		state->membank("bank3")->set_base(state->m_memory + 0xa000);
		space.unmap_write(0xa000, 0xafff);

		state->membank("bank4")->set_base(state->m_memory + 0xb000);
		space.unmap_write(0xb000, 0xbfff);

		state->membank("bank6")->set_base(state->m_memory + 0xc000);
		space.unmap_write(0xc000, 0xe7ff);

		space.install_legacy_read_handler(0xe800, 0xefff, FUNC(cbm8096_io_r));
		space.install_legacy_write_handler(0xe800, 0xefff, FUNC(cbm8096_io_w));

		state->membank("bank8")->set_base(state->m_memory + 0xf000);
		space.unmap_write(0xf000, 0xffef);

		state->membank("bank9")->set_base(state->m_memory + 0xfff1);
		space.unmap_write(0xfff1, 0xffff);
	}
}

READ8_HANDLER( superpet_r )
{
	return 0xff;
}

WRITE8_HANDLER( superpet_w )
{
	pet_state *state = space.machine().driver_data<pet_state>();
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 3: 1 pull down diagnostic pin on the userport
			   1: 1 if jumpered programable ram r/w
			   0: 0 if jumpered programable m6809, 1 m6502 selected */
			break;

		case 4:
		case 5:
			state->m_spet.bank = data & 0xf;
			state->membank("bank1")->configure_entries(0, 16, state->m_supermemory, 0x1000);
			state->membank("bank1")->set_entry(state->m_spet.bank);
			/* 7 low writeprotects systemlatch */
			break;

		case 6:
		case 7:
			state->m_spet.rom = data & 1;
			break;
	}
}

TIMER_CALLBACK_MEMBER(pet_state::pet_interrupt)
{
	pia6821_device *pia_0 = machine().device<pia6821_device>("pia_0");

	pia_0->cb1_w(m_pia_level);
	m_pia_level = !m_pia_level;
}


static void pet_common_driver_init( running_machine &machine )
{
	int i;
	pet_state *state = machine.driver_data<pet_state>();

	state->m_font = 0;

	state->m_pet_basic1 = 0;
	state->m_superpet = 0;
	state->m_cbm8096 = 0;

	machine.device("maincpu")->memory().space(AS_PROGRAM).install_readwrite_bank(0x0000, machine.device<ram_device>(RAM_TAG)->size() - 1, "bank10");
	state->membank("bank10")->set_base(state->m_memory);

	if (machine.device<ram_device>(RAM_TAG)->size() < 0x8000)
	{
		machine.device("maincpu")->memory().space(AS_PROGRAM).nop_readwrite(machine.device<ram_device>(RAM_TAG)->size(), 0x7FFF);
	}

	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < machine.device<ram_device>(RAM_TAG)->size(); i += 0x40)
	{
		memset (state->m_memory + i, i & 0x40 ? 0 : 0xff, 0x40);
	}

	/* pet clock */
	machine.scheduler().timer_pulse(attotime::from_msec(10), timer_expired_delegate(FUNC(pet_state::pet_interrupt),state));
}


DRIVER_INIT_MEMBER(pet_state,pet2001)
{
	m_memory.set_target(machine().device<ram_device>(RAM_TAG)->pointer(),m_memory.bytes());
	pet_common_driver_init(machine());
	m_pet_basic1 = 1;
	pet_vh_init(machine());
}

DRIVER_INIT_MEMBER(pet_state,pet)
{
	m_memory.set_target(machine().device<ram_device>(RAM_TAG)->pointer(),m_memory.bytes());
	pet_common_driver_init(machine());
	pet_vh_init(machine());
}

DRIVER_INIT_MEMBER(pet_state,pet80)
{
	m_memory.set_target(memregion("maincpu")->base(),m_memory.bytes());

	pet_common_driver_init(machine());
	m_cbm8096 = 1;
	m_videoram.set_target(&m_memory[0x8000],m_videoram.bytes());
	pet80_vh_init(machine());

}

DRIVER_INIT_MEMBER(pet_state,superpet)
{
	m_memory.set_target(machine().device<ram_device>(RAM_TAG)->pointer(),m_memory.bytes());
	pet_common_driver_init(machine());
	m_superpet = 1;

	m_supermemory = auto_alloc_array(machine(), UINT8, 0x10000);

	membank("bank1")->configure_entries(0, 16, m_supermemory, 0x1000);
	membank("bank1")->set_entry(0);

	superpet_vh_init(machine());
}

void pet_state::machine_reset()
{
	if (m_superpet)
	{
		m_spet.rom = 0;
		if (machine().root_device().ioport("CFG")->read() & 0x04)
		{
			machine().device("maincpu")->execute().set_input_line(INPUT_LINE_HALT, 1);
			machine().device("maincpu")->execute().set_input_line(INPUT_LINE_HALT, 0);
			m_font = 2;
		}
		else
		{
			machine().device("maincpu")->execute().set_input_line(INPUT_LINE_HALT, 0);
			machine().device("maincpu")->execute().set_input_line(INPUT_LINE_HALT, 1);
			m_font = 0;
		}
	}

	if (m_cbm8096)
	{
		if (machine().root_device().ioport("CFG")->read() & 0x08)
		{
			machine().device("maincpu")->memory().space(AS_PROGRAM).install_legacy_write_handler(0xfff0, 0xfff0, FUNC(cbm8096_w));
		}
		else
		{
			machine().device("maincpu")->memory().space(AS_PROGRAM).nop_write(0xfff0, 0xfff0);
		}
		cbm8096_w(machine().device("maincpu")->memory().space(AS_PROGRAM), 0, 0);
	}

//removed   cbm_drive_0_config (machine().root_device().ioport("CFG")->read() & 2 ? IEEE : 0, 8);
//removed   cbm_drive_1_config (machine().root_device().ioport("CFG")->read() & 1 ? IEEE : 0, 9);
	machine().device("maincpu")->reset();

	m_ieee->ren_w(0);
	m_ieee->ifc_w(0);
	m_ieee->ifc_w(1);
}


INTERRUPT_GEN_MEMBER(pet_state::pet_frame_interrupt)
{
	if (m_superpet)
	{
		if (ioport("CFG")->read() & 0x04)
		{
			device.execute().set_input_line(INPUT_LINE_HALT, 1);
			device.execute().set_input_line(INPUT_LINE_HALT, 0);
			m_font |= 2;
		}
		else
		{
			device.execute().set_input_line(INPUT_LINE_HALT, 0);
			device.execute().set_input_line(INPUT_LINE_HALT, 1);
			m_font &= ~2;
		}
	}

	set_led_status (machine(),1, machine().root_device().ioport("SPECIAL")->read() & 0x80 ? 1 : 0);     /* Shift Lock */
}


/***********************************************

    PET Cartridges

***********************************************/

static DEVICE_IMAGE_LOAD(pet_cart)
{
	pet_state *state = image.device().machine().driver_data<pet_state>();
	UINT32 size = image.length();
	const char *filetype = image.filetype();
	int address = 0;

	/* Assign loading address according to extension */
	if (!mame_stricmp(filetype, "90"))
		address = 0x9000;
	else if (!mame_stricmp(filetype, "a0"))
		address = 0xa000;
	else if (!mame_stricmp(filetype, "b0"))
		address = 0xb000;

	logerror("Loading cart %s at %.4x size:%.4x\n", image.filename(), address, size);

	image.fread(state->m_memory + address, size);

	return IMAGE_INIT_PASS;
}

MACHINE_CONFIG_FRAGMENT(pet_cartslot)
	MCFG_CARTSLOT_ADD("cart1")
	MCFG_CARTSLOT_EXTENSION_LIST("90,a0,b0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)

	MCFG_CARTSLOT_ADD("cart2")
	MCFG_CARTSLOT_EXTENSION_LIST("90,a0,b0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)
MACHINE_CONFIG_END

// 2010-08, FP: this is used by CBM40 & CBM80, and I actually have only found .prg files for these... does cart dumps exist?
MACHINE_CONFIG_FRAGMENT(pet4_cartslot)
	MCFG_CARTSLOT_MODIFY("cart1")
	MCFG_CARTSLOT_EXTENSION_LIST("a0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)

	MCFG_CARTSLOT_MODIFY("cart2")
	MCFG_CARTSLOT_EXTENSION_LIST("a0")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(pet_cart)
MACHINE_CONFIG_END
