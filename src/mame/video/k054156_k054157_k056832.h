#pragma once
#ifndef __K056832_H__
#define __K056832_H__

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#include "video/k055555.h"// still needs k055555_get_palette_index


typedef void (*k056832_callback)(running_machine &machine, int layer, int *code, int *color, int *flags);


#define MCFG_K056832_ADD(_tag, _interface) \
	MCFG_DEVICE_ADD(_tag, K056832, 0) \
	MCFG_DEVICE_CONFIG(_interface)

#define MCFG_K056832_ADD_NOINTF(_tag ) \
	MCFG_DEVICE_ADD(_tag, K056832, 0)


struct k056832_interface
{
	const char         *m_gfx_memory_region;
	int                m_gfx_num;
	int                m_bpp;
	int                m_big;
	int                m_djmain_hack;
	int                m_deinterleave;
	k056832_callback   m_callback;

	const char         *m_k055555_tag;    // tbyahhoo uses the k056832 together with a k055555
};



#define K056832_PAGE_COUNT 16

/* bit depths for the 56832 */
#define K056832_BPP_4   0
#define K056832_BPP_5   1
#define K056832_BPP_6   2
#define K056832_BPP_8   3
#define K056832_BPP_4dj 4
#define K056832_BPP_8LE 5
#define K056832_BPP_8TASMAN 6

#define K056832_DRAW_FLAG_MIRROR      0x00800000
#define K056382_DRAW_FLAG_FORCE_XYSCROLL        0x00800000


class k056832_device : public device_t,
										public k056832_interface
{
public:
	k056832_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~k056832_device()
	{
		m_k055555 = 0;
	}

	// static configuration
	static void static_set_gfxdecode_tag(device_t &device, const char *tag);
	static void static_set_palette_tag(device_t &device, const char *tag);

	void SetExtLinescroll();    /* Lethal Enforcers */

	DECLARE_READ16_MEMBER( ram_word_r );
	DECLARE_WRITE16_MEMBER( ram_word_w );
	DECLARE_READ16_MEMBER( ram_half_word_r );
	DECLARE_WRITE16_MEMBER( ram_half_word_w );
	DECLARE_READ16_MEMBER( k_5bpp_rom_word_r );
	DECLARE_READ32_MEMBER( k_5bpp_rom_long_r );
	DECLARE_READ32_MEMBER( k_6bpp_rom_long_r );
	DECLARE_READ16_MEMBER( rom_word_r );
	DECLARE_READ16_MEMBER( mw_rom_word_r );
	DECLARE_READ16_MEMBER( bishi_rom_word_r );
	DECLARE_READ16_MEMBER( old_rom_word_r );
	DECLARE_READ16_MEMBER( rom_word_8000_r );
	DECLARE_WRITE16_MEMBER( word_w ); // "VRAM" registers
	DECLARE_WRITE16_MEMBER( b_word_w );
	DECLARE_READ8_MEMBER( ram_code_lo_r );
	DECLARE_READ8_MEMBER( ram_code_hi_r );
	DECLARE_READ8_MEMBER( ram_attr_lo_r );
	DECLARE_READ8_MEMBER( ram_attr_hi_r );
	DECLARE_WRITE8_MEMBER( ram_code_lo_w );
	DECLARE_WRITE8_MEMBER( ram_code_hi_w );
	DECLARE_WRITE8_MEMBER( ram_attr_lo_w );
	DECLARE_WRITE8_MEMBER( ram_attr_hi_w );
	DECLARE_WRITE8_MEMBER( write );
	DECLARE_WRITE8_MEMBER( b_w );
	void mark_plane_dirty(int num);
	void mark_all_tilemaps_dirty();
	void tilemap_draw(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, int num, UINT32 flags, UINT32 priority);
	void tilemap_draw(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect, int num, UINT32 flags, UINT32 priority);
	void tilemap_draw_dj(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect, int layer, UINT32 flags, UINT32 priority);
	void set_layer_association(int status);
	int  get_layer_association();
	void set_layer_offs(int layer, int offsx, int offsy);
	void set_lsram_page(int logical_page, int physical_page, int physical_offset);
	void linemap_enable(int enable);
	int  is_irq_enabled(int irqline);
	void read_avac(int *mode, int *data);
	int  read_register(int regnum);
	int get_current_rambank();
	int get_lookup(int bits); /* Asterix */
	void set_tile_bank(int bank); /* Asterix */

	DECLARE_READ32_MEMBER( ram_long_r );
	DECLARE_READ32_MEMBER( rom_long_r );
	DECLARE_WRITE32_MEMBER( ram_long_w );
	DECLARE_READ32_MEMBER( unpaged_ram_long_r );
	DECLARE_WRITE32_MEMBER( unpaged_ram_long_w );
	DECLARE_WRITE32_MEMBER( long_w );
	DECLARE_WRITE32_MEMBER( b_long_w );

	DECLARE_READ16_MEMBER( word_r );        // VACSET
	DECLARE_READ16_MEMBER( b_word_r );      // VSCCS  (board dependent)
	DECLARE_READ32_MEMBER( long_r );        // VACSET


protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();

private:
	// internal state
	tilemap_t   *m_tilemap[K056832_PAGE_COUNT];
	bitmap_ind16  *m_pixmap[K056832_PAGE_COUNT];

	UINT16    m_regs[0x20];   // 157/832 regs group 1
	UINT16    m_regsb[4]; // 157/832 regs group 2, board dependent

	UINT8 *   m_rombase;  // pointer to tile gfx data
	UINT16 *  m_videoram;
	int       m_num_gfx_banks;    // depends on size of graphics ROMs
	int       m_cur_gfx_banks;        // cached info for K056832_regs[0x1a]









	// ROM readback involves reading 2 halves of a word
	// from the same location in a row.  Reading the
	// RAM window resets this state so you get the first half.
	int       m_rom_half;

	// locally cached values
	int       m_layer_assoc_with_page[K056832_PAGE_COUNT];
	int       m_layer_offs[8][2];
	int       m_lsram_page[8][2];
	INT32     m_x[8]; // 0..3 left
	INT32     m_y[8]; // 0..3 top
	INT32     m_w[8]; // 0..3 width  -> 1..4 pages
	INT32     m_h[8]; // 0..3 height -> 1..4 pages
	INT32     m_dx[8];    // scroll
	INT32     m_dy[8];    // scroll
	UINT32    m_line_dirty[K056832_PAGE_COUNT][8];
	UINT8     m_all_lines_dirty[K056832_PAGE_COUNT];
	UINT8     m_page_tile_mode[K056832_PAGE_COUNT];
	int       m_last_colorbase[K056832_PAGE_COUNT];
	UINT8     m_layer_tile_mode[8];
	int       m_default_layer_association;
	int       m_layer_association;
	int       m_active_layer;
	int       m_selected_page;
	int       m_selected_page_x4096;
	int       m_linemap_enabled;
	int       m_use_ext_linescroll;
	int       m_uses_tile_banks, m_cur_tile_bank;








	k055555_device *m_k055555;  /* used to choose colorbase */

	void get_tile_info(  tile_data &tileinfo, int tile_index, int pageIndex );

	TILE_GET_INFO_MEMBER(get_tile_info0);
	TILE_GET_INFO_MEMBER(get_tile_info1);
	TILE_GET_INFO_MEMBER(get_tile_info2);
	TILE_GET_INFO_MEMBER(get_tile_info3);
	TILE_GET_INFO_MEMBER(get_tile_info4);
	TILE_GET_INFO_MEMBER(get_tile_info5);
	TILE_GET_INFO_MEMBER(get_tile_info6);
	TILE_GET_INFO_MEMBER(get_tile_info7);
	TILE_GET_INFO_MEMBER(get_tile_info8);
	TILE_GET_INFO_MEMBER(get_tile_info9);
	TILE_GET_INFO_MEMBER(get_tile_infoa);
	TILE_GET_INFO_MEMBER(get_tile_infob);
	TILE_GET_INFO_MEMBER(get_tile_infoc);
	TILE_GET_INFO_MEMBER(get_tile_infod);
	TILE_GET_INFO_MEMBER(get_tile_infoe);
	TILE_GET_INFO_MEMBER(get_tile_infof);

	void mark_page_dirty(int page);
	void update_page_layout();
	void change_rambank();
	void change_rombank();
	void postload();
	int rom_read_b(int offset, int blksize, int blksize2, int zerosec);

	template<class _BitmapClass>
	int update_linemap(screen_device &screen, _BitmapClass &bitmap, int page, int flags);

	template<class _BitmapClass>
	void tilemap_draw_common(screen_device &screen, _BitmapClass &bitmap, const rectangle &cliprect, int layer, UINT32 flags, UINT32 priority);

	void create_gfx(running_machine &machine, const char *gfx_memory_region, int bpp, int big);
	void create_tilemaps(running_machine &machine);
	void finalize_init(running_machine &machine);

	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
public:

	// todo: collapse these into above
	void altK056832_vh_start(running_machine &machine, const char *gfx_memory_region, int bpp, int big,
				int (*scrolld)[4][2],
				void (*callback)(running_machine &machine, int layer, int *code, int *color, int *flags),
				int djmain_hack);

	void K056832_set_k055555(k055555_device* mode); // k055555 hook


	void m_tilemap_draw(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect, int num, UINT32 flags, UINT32 priority);
private:
	int altK056832_update_linemap(screen_device &screen, bitmap_rgb32 &bitmap, int page, int flags);


};

extern const device_type K056832;




#define MCFG_K056832_GFXDECODE(_gfxtag) \
	k056832_device::static_set_gfxdecode_tag(*device, "^" _gfxtag);

#define MCFG_K056832_PALETTE(_palette_tag) \
	k056832_device::static_set_palette_tag(*device, "^" _palette_tag);


#endif
