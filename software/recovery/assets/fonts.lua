local MAX_WIDTH = 8
local MAX_HEIGHT = 8
local ROM_OFFSET_SHIFT = 0

local bdf = dofile("lib/bdf.lua")
local process = require("wf.api.v1.process")
local tablex = require("pl.tablex")

local function table_to_string(n)
    return string.char(table.unpack(n))
end

local function build_font(name, fonts, Y_OFFSET, is_allowed_char)
    local chars = {}
    local i = 0
    local max_glyph_id = 0

    for _, font in pairs(fonts) do
        if Y_OFFSET == nil then
            local a_char = font.chars[91] -- [
            local a_y = font.ascent - a_char.y - a_char.height
            Y_OFFSET = -a_y
        end

        for id, char in pairs(font.chars) do
            if chars[id] == nil and is_allowed_char(id, font) then
                if id == 0x20 then
                    char.x = 0
                    char.width = 2
                    char.y = 0
                    char.height = 0
                end
                local bitmap = char.bitmap
                local bitmap_empty = true
                for i=1,#char.bitmap do
                    if char.bitmap[i] ~= 0 then
                        bitmap_empty = false
                        break
                    end
                end
                if bitmap_empty then
                    char.y = 0
                    char.height = 0
                end
                if char.height > MAX_HEIGHT then
                    -- remove empty rows
                    bitmap = {}
                    for i=1,#char.bitmap do
                        if char.bitmap[i] ~= 0 then
                            table.insert(bitmap, char.bitmap[i])
                        end
                    end

                    if #bitmap > MAX_HEIGHT then
                        print(char.name .. " too tall, skipping")
                        goto nextchar
                    end
                end
                if char.width > MAX_WIDTH then
                    print(char.name .. " too wide, skipping")
                    goto nextchar
                end

                -- calculate xofs, yofs, width, height
                local res = {}
                res.width = char.width
                res.height = #bitmap
                res.bitmap = {}
                -- pack ROM data
                local x = 0
                local i = 0
                for iy=1,char.height do
                    local b = char.bitmap[iy] >> (((char.width + 7) & 0xFFF8) - char.width)
                    local m = 1
                    for ix=1,char.width do
                        if (b & m) ~= 0 then
                            x = x | (1 << i)
                        end
                        i = i + 1
                        m = m << 1
                    end
                    table.insert(res.bitmap, x)
                    x = 0
                    i = 0
                end
                res.x = char.x
                res.y = font.ascent - char.y - char.height + Y_OFFSET
                if res.x < 0 then res.x = 0 end
                if res.y < 0 then res.y = 0 end
                if (res.y + res.height) > MAX_HEIGHT then
                    res.y = MAX_HEIGHT - res.height
                end
                if (res.x > 0 or res.width > 0) then
                    chars[id] = res
                    if id > max_glyph_id then
                        max_glyph_id = id
                    end
                end
            end
            ::nextchar::
        end
    end

    local rom_datas = {}

    local function add_char(char)
        table.insert(rom_datas, char.x + char.width + 1)
        for i=0,7 do
             if (i >= char.y and (i - char.y) < char.height) then
                 table.insert(rom_datas, char.bitmap[i - char.y + 1] << char.x)
             else
                 table.insert(rom_datas, 0)
             end
        end
    end

    for i=32,126 do add_char(chars[i]) end

    process.emit_symbol(name, table_to_string(rom_datas))
end

local boring7 = bdf.parse("boring7.bdf")
build_font("font8_bitmap", {boring7}, 0, function(ch, font) return true end)
