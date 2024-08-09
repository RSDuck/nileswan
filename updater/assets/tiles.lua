local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")
local zx0 = require("wf.api.v1.process.tools.zx0")

local tileset = superfamiconv.convert_tileset(
	"tiles.png",
	superfamiconv.config()
		:mode("ws"):bpp(2)
		:color_zero("#ffffff")
		:no_discard():no_flip()
)

process.emit_symbol("gfx_tiles", zx0.compress(tileset.tiles, zx0.config({verbose=true})))
