local process = require("wf.api.v1.process")
local lzsa = require("wf.api.v1.process.tools.lzsa")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

local tileset = superfamiconv.convert_tileset(
	"tiles.png",
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
)

process.emit_symbol("gfx_tiles", lzsa.compress2(tileset.tiles))
