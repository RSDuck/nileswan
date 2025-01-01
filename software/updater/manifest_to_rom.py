#!/usr/bin/python3
#
# Copyright (c) 2024 Adrian Siekierka
#
# Nileswan Updater is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Nileswan Updater is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with Nileswan Updater. If not, see <https://www.gnu.org/licenses/>.

import argparse, crc, os, struct, subprocess, sys

parser = argparse.ArgumentParser(prog='manifest_to_rom', description='Create updater ROM from manifest')
parser.add_argument('input_rom', help='Input ROM file (updater_base.ws)')
parser.add_argument('manifest', help='File describing the update contents')
parser.add_argument('output_rom', help='Output ROM file (.ws)')
args = parser.parse_args()

updater_base_data = None
with open(args.input_rom, 'rb') as file:
    updater_base_data = file.read()

rule_data = bytearray()
file_position = 0x10000 - ((len(updater_base_data) + 15) >> 4)
data_at_position = {}

crc16 = crc.Calculator(crc.Configuration(
    width=16,
    polynomial=0x1021,
    init_value=0x0000,
    final_xor_value=0x0000,
    reverse_input=False,
    reverse_output=False,
))

version = [9, 9, 9]

with open(args.manifest, 'r') as rules:
    for line in rules:
        rule = line.strip()
        if rule.startswith("#"):
            continue
        rule = rule.split(" ")
        rule_name = rule[0]
        rule_map = {}
        for i in range(0, len(rule), 2):
            rule_map[rule[i]] = rule[i+1]
        if rule_name == 'FLASH':
            data = None
            with open(rule_map['FLASH'], 'rb') as file:
                data = file.read()

            file_position = file_position - ((len(data) + 15) >> 4)

            data_at_position[file_position] = data
            rule_data += bytearray(struct.pack("<BHHIH",
                0x01, file_position, len(data), int(rule_map['FLASH']), crc16.checksum(data)))
        elif rule_name == 'PACKED_FLASH':
            subprocess.run(["rm", "temp.bin"])
            subprocess.run(["wf-zx0-salvador", "-v", rule[1], "temp.bin"])
            unpacked_data = None
            with open(rule_map['PACKED_FLASH'], 'rb') as file:
                unpacked_data = file.read()
            data = None
            with open('temp.bin', 'rb') as file:
                data = file.read()
            subprocess.run(["rm", "temp.bin"])

            file_position = file_position - ((len(data) + 15) >> 4)
            flash_position = int(rule_map['AT'])
            if flash_position < 0:
                flash_position = (-flash_position) - len(data)

            data_at_position[file_position] = data

            rule_data += bytearray(struct.pack("<BHHIH",
                0x02, file_position, len(unpacked_data), flash_position, crc16.checksum(unpacked_data)))
        elif rule_name == 'VERSION':
            version[0] = int(rule_map['VERSION'])
            version[1] = int(rule_map['MINOR'])
            version[2] = int(rule_map['PATCH'])
        else:
            raise Exception(f"Unknown rule: {rule[0]}")
    rule_data.append(0)

rule_header = bytearray(struct.pack("<HHH", version[0], version[1], version[2]))
rule_data = rule_header + rule_data

file_position = (file_position - ((len(rule_data) + 4 + 15) >> 4)) & 0xFF00
rule_position = file_position
if file_position > 0xE000:
    file_position = 0xE000

# FIXME!!
file_position = 0x0000

file_length = (0x10000 - file_position) * 16

with open(args.output_rom, 'wb') as file:
    file.write(b'\xFF' * (file_length - len(updater_base_data)))
    file.write(updater_base_data)

    file.seek(file_length - 16 + 6)
    file.write(struct.pack("<B", rule_position >> 8))

    file.seek((rule_position - file_position) * 16)
    file.write(struct.pack("<HH", len(rule_data), crc16.checksum(rule_data)))
    file.write(rule_data)

    for k, v in data_at_position.items():
        file.seek((k - file_position) * 16)
        file.write(v)
