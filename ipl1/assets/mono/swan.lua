local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

for i, file in pairs(process.inputs(".png")) do
	local tilemap = superfamiconv.convert_tilemap(
		file,
		superfamiconv.config()
			:mode("ws"):bpp(2)
			:color_zero("#aaaaaa")
			:tile_base(384):palette_base(12)
	)
	process.emit_symbol("gfx_mono_" .. process.symbol(file), tilemap)
end
