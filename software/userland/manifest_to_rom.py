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

import argparse, crc, hashlib, os, struct, subprocess, sys

parser = argparse.ArgumentParser(prog='manifest_to_rom', description='Create updater ROM from manifest')
parser.add_argument('input_rom', help='Input ROM file (updater_base.ws)')
parser.add_argument('manifest', help='File describing the update contents')
parser.add_argument('output_rom', help='Output ROM file (.ws)')
args = parser.parse_args()

updater_base_data = None
with open(args.input_rom, 'rb') as file:
    updater_base_data = file.read()

rule_data = bytearray()
start_segment = 0x10000 - ((len(updater_base_data) + 15) >> 4)
data_at_position = {}

crc16 = crc.Calculator(crc.Configuration(
    width=16,
    polynomial=0x1021,
    init_value=0x0000,
    final_xor_value=0x0000,
    reverse_input=False,
    reverse_output=False,
))

MAXIMUM_PART_SIZE = 49152
FLASH_SECTOR_SIZE = 256
version = {
    "major": 0,
    "minor": 1,
    "patch": 0,
    "commit": "0000000000000000000000000000000000000000",
    "digest": "0000000000000000000000000000000000000000000000000000000000000000"
}

digest_hash = hashlib.new("sha256")

try:
    git_out = subprocess.run(["git", "rev-parse", "--short=40", "HEAD"], stdout=subprocess.PIPE)
    commit_out = git_out.stdout.decode("utf-8").strip()
    if len(commit_out) == 40:
        version["commit"] = commit_out
except e:
    pass

def pad(data, right):
    diff = FLASH_SECTOR_SIZE - int(len(data) % FLASH_SECTOR_SIZE)
    if diff == FLASH_SECTOR_SIZE:
        return data
    if right:
        data = data + b'\xFF'*diff
    else:
        data = b'\xFF'*diff + data
    return data

def split_data_by_part_size(flash_position, data):
    while len(data) > MAXIMUM_PART_SIZE:
        yield (flash_position, data[0:MAXIMUM_PART_SIZE])
        flash_position += MAXIMUM_PART_SIZE
        data = data[MAXIMUM_PART_SIZE:]

    yield (flash_position, data)

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

        board_revision = 0xFFFF
        if 'BOARD_REVISION' in rule_map:
            board_revision = int(rule_map['BOARD_REVISION'])

        if rule_name == 'FLASH':
            data = None
            with open(rule_map['FLASH'], 'rb') as file:
                data = file.read()

            flash_position = int(rule_map['AT'])
            data = pad(data, flash_position >= 0)
            if flash_position < 0:
                flash_position = (-flash_position) - len(data)

            for (flash_position, data) in split_data_by_part_size(flash_position, data):
                if (flash_position & 0xFF) != 0:
                    raise Exception("File {rule[1]} cannot be flashed at unaligned position {flash_position}")

                start_segment = start_segment - ((len(data) + 15) >> 4)
                data_at_position[start_segment] = data

                rule_data += bytearray(struct.pack("<BHHIHH",
                    0x02, start_segment, len(data), flash_position, crc16.checksum(data), board_revision))
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

            flash_position = int(rule_map['AT'])
            if flash_position < 0:
                flash_position = (-flash_position) - len(data)

            if len(unpacked_data) > 65535:
                raise Exception("File {rule[1]} size too large ({len(unpacked_data)} > 65535)")
            # TODO: Pad to flash sector size (or detect issue)
            if (flash_position & 0xFF) != 0:
                raise Exception("File {rule[1]} cannot be flashed at unaligned position {flash_position}")

            start_segment = start_segment - ((len(data) + 15) >> 4)
            data_at_position[start_segment] = data

            rule_data += bytearray(struct.pack("<BHHIHH",
                0x03, start_segment, len(unpacked_data), flash_position, crc16.checksum(unpacked_data), board_revision))
        elif rule_name == 'MCU_FLASH':
            data = None
            with open(rule_map['MCU_FLASH'], 'rb') as file:
                data = file.read()

            flash_position = int(rule_map['AT'])

            for (flash_position, data) in split_data_by_part_size(flash_position, data):
                if (flash_position & 0x7FF) != 0:
                    raise Exception("File {rule[1]} cannot be flashed at unaligned position {flash_position}")

                start_segment = start_segment - ((len(data) + 15) >> 4)
                data_at_position[start_segment] = data

                rule_data += bytearray(struct.pack("<BHHIHH",
                    0x04, start_segment, len(data), flash_position, crc16.checksum(data), board_revision))
        else:
            raise Exception(f"Unknown rule: {rule[0]}")
    rule_data.append(0)

for k, v in data_at_position.items():
    digest_hash.update(v)
version["digest"] = digest_hash.hexdigest()

rule_header = bytearray(struct.pack("<BBHHHHH", 70, 87, version["major"], version["minor"], version["patch"], 0, 0)) + bytearray.fromhex(version["commit"]) + bytearray.fromhex(version["digest"])
rule_data = rule_header + rule_data

start_segment = (start_segment - ((len(rule_data) + 4 + 15) >> 4)) & 0xFF00
start_segment_rounded = start_segment
if start_segment_rounded >= 0xE000:
    start_segment_rounded = 0xE000
elif start_segment_rounded >= 0xC000:
    start_segment_rounded = 0xC000
elif start_segment_rounded >= 0x8000:
    start_segment_rounded = 0x8000
else:
    start_segment_rounded = 0x0000

file_length = (0x10000 - start_segment_rounded) * 16

with open(args.output_rom, 'wb') as file:
    file.write(b'\xFF' * (file_length - len(updater_base_data)))
    file.write(updater_base_data)

    file.seek(file_length - 16 + 6)
    file.write(struct.pack("<B", start_segment >> 8))

    file.seek((start_segment - start_segment_rounded) * 16)
    file.write(struct.pack("<HH", len(rule_data), crc16.checksum(rule_data)))
    file.write(rule_data)

    for k, v in data_at_position.items():
        file.seek((k - start_segment_rounded) * 16)
        file.write(v)
