local process = require("wf.api.v1.process")
local superfamiconv = require("wf.api.v1.process.tools.superfamiconv")

for i, file in pairs(process.inputs(".png")) do
	local tilemap = superfamiconv.convert_tilemap(
		file,
		superfamiconv.config()
			:mode("ws"):bpp(2)
			:tile_base(1):palette_base(0)
	)
	process.emit_symbol("gfx_mono_" .. process.symbol(file), tilemap)
end
