/***************************************************************************

    Mitsubishi Multi 16

****************************************************************************/

#include "emu.h"
#include "cpu/i86/i86.h"
#include "video/mc6845.h"
#include "machine/pic8259.h"


class multi16_state : public driver_device
{
public:
	multi16_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_pic(*this, "pic8259"),
	m_crtc(*this, "crtc"),
		m_palette(*this, "palette") 
	,
		m_p_vram(*this, "p_vram"){ }

	required_device<cpu_device> m_maincpu;
	required_device<pic8259_device> m_pic;
	required_device<mc6845_device> m_crtc;
	required_device<palette_device> m_palette;
	DECLARE_WRITE8_MEMBER(multi16_6845_address_w);
	DECLARE_WRITE8_MEMBER(multi16_6845_data_w);
	DECLARE_WRITE_LINE_MEMBER(multi16_set_int_line);
	required_shared_ptr<UINT16> m_p_vram;
	UINT8 m_crtc_vreg[0x100],m_crtc_index;
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update_multi16(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	IRQ_CALLBACK_MEMBER(multi16_irq_callback);
};


void multi16_state::video_start()
{
}

#define mc6845_h_char_total     (m_crtc_vreg[0])
#define mc6845_h_display        (m_crtc_vreg[1])
#define mc6845_h_sync_pos       (m_crtc_vreg[2])
#define mc6845_sync_width       (m_crtc_vreg[3])
#define mc6845_v_char_total     (m_crtc_vreg[4])
#define mc6845_v_total_adj      (m_crtc_vreg[5])
#define mc6845_v_display        (m_crtc_vreg[6])
#define mc6845_v_sync_pos       (m_crtc_vreg[7])
#define mc6845_mode_ctrl        (m_crtc_vreg[8])
#define mc6845_tile_height      (m_crtc_vreg[9]+1)
#define mc6845_cursor_y_start   (m_crtc_vreg[0x0a])
#define mc6845_cursor_y_end     (m_crtc_vreg[0x0b])
#define mc6845_start_addr       (((m_crtc_vreg[0x0c]<<8) & 0x3f00) | (m_crtc_vreg[0x0d] & 0xff))
#define mc6845_cursor_addr      (((m_crtc_vreg[0x0e]<<8) & 0x3f00) | (m_crtc_vreg[0x0f] & 0xff))
#define mc6845_light_pen_addr   (((m_crtc_vreg[0x10]<<8) & 0x3f00) | (m_crtc_vreg[0x11] & 0xff))
#define mc6845_update_addr      (((m_crtc_vreg[0x12]<<8) & 0x3f00) | (m_crtc_vreg[0x13] & 0xff))


UINT32 multi16_state::screen_update_multi16(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int x,y;
	int count;
	int xi;

	count = 0;

	for(y=0;y<mc6845_v_display*8;y++)
	{
		for(x=0;x<(mc6845_h_display*8)/16;x++)
		{
			for(xi=0;xi<16;xi++)
			{
				int dot = (BITSWAP16(m_p_vram[count],7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8) >> (15-xi)) & 0x1;

				if(screen.visible_area().contains(x*16+xi, y))
					bitmap.pix16(y, x*16+xi) = m_palette->pen(dot);
			}

			count++;
		}
	}

	return 0;
}

static ADDRESS_MAP_START(multi16_map, AS_PROGRAM, 16, multi16_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000,0x7ffff) AM_RAM
	AM_RANGE(0xd8000,0xdffff) AM_RAM AM_SHARE("p_vram")
	AM_RANGE(0xe0000,0xeffff) AM_RAM
	AM_RANGE(0xf0000,0xf3fff) AM_MIRROR(0xc000) AM_ROM AM_REGION("ipl", 0)
ADDRESS_MAP_END

WRITE8_MEMBER( multi16_state::multi16_6845_address_w )
{
	m_crtc_index = data;
	m_crtc->address_w(space, offset, data);
}

WRITE8_MEMBER( multi16_state::multi16_6845_data_w )
{
	m_crtc_vreg[m_crtc_index] = data;
	m_crtc->register_w(space, offset, data);
}

static ADDRESS_MAP_START(multi16_io, AS_IO, 16, multi16_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x02, 0x03) AM_DEVREADWRITE8("pic8259", pic8259_device, read, write, 0xffff) // i8259
	AM_RANGE(0x40, 0x41) AM_WRITE8(multi16_6845_address_w, 0x00ff)
	AM_RANGE(0x40, 0x41) AM_WRITE8(multi16_6845_data_w, 0xff00)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( multi16 )
INPUT_PORTS_END

IRQ_CALLBACK_MEMBER(multi16_state::multi16_irq_callback)
{
	return machine().device<pic8259_device>("pic8259")->acknowledge();
}

WRITE_LINE_MEMBER( multi16_state::multi16_set_int_line )
{
	//printf("%02x\n",interrupt);
	m_maincpu->set_input_line(0, state ? HOLD_LINE : CLEAR_LINE);
}

void multi16_state::machine_start()
{
	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(multi16_state::multi16_irq_callback),this));
}


void multi16_state::machine_reset()
{
}


static MC6845_INTERFACE( mc6845_intf )
{
	false,      /* show border area */
	0,0,0,0,    /* visarea adjustment */
	8,          /* number of pixels per video memory address */
	NULL,       /* before pixel update callback */
	NULL,       /* row update callback */
	NULL,       /* after pixel update callback */
	DEVCB_NULL, /* callback for display state changes */
	DEVCB_NULL, /* callback for cursor state changes */
	DEVCB_NULL, /* HSYNC callback */
	DEVCB_NULL, /* VSYNC callback */
	NULL        /* update address callback */
};

static MACHINE_CONFIG_START( multi16, multi16_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", I8086, 8000000)
	MCFG_CPU_PROGRAM_MAP(multi16_map)
	MCFG_CPU_IO_MAP(multi16_io)


	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MCFG_SCREEN_UPDATE_DRIVER(multi16_state, screen_update_multi16)
	MCFG_PALETTE_ADD("palette", 8)

	/* Devices */
	MCFG_MC6845_ADD("crtc", H46505, "screen", 16000000/5, mc6845_intf)    /* unknown clock, hand tuned to get ~60 fps */
	MCFG_PIC8259_ADD( "pic8259", WRITELINE(multi16_state, multi16_set_int_line), GND, NULL )
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( multi16 )
	ROM_REGION( 0x4000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x4000, CRC(5beb5e94) SHA1(d3b9dc9a08995a0f26af9671893417e795370306))
ROM_END

/* Driver */

/*    YEAR  NAME     PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY     FULLNAME       FLAGS */
COMP( 1986, multi16, 0,      0,       multi16,   multi16, driver_device, 0,   "Mitsubishi", "Multi 16", GAME_NOT_WORKING | GAME_NO_SOUND)
