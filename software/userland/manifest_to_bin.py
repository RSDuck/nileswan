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

parser = argparse.ArgumentParser(prog='manifest_to_bin', description='Create SPI flash file from manifest')
parser.add_argument('manifest', help='File describing the update contents')
parser.add_argument('output_bin', help='Output BIN file (.bin)')
args = parser.parse_args()

write_at = {}
max_size = 0

updater_base_data = None
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
        if rule_name == 'FLASH' or rule_name == 'PACKED_FLASH':
            data = None
            with open(rule_map[rule_name], 'rb') as file:
                data = file.read()
            pos = int(rule_map['AT'])
            if pos < 0:
                pos = (-pos) - len(data)
            write_at[pos] = data
            max_size = max(max_size, pos + len(data))

with open(args.output_bin, 'wb') as fo:
    fo.write(b'\xFF' * max_size)
    for k, v in write_at.items():
        fo.seek(k)
        fo.write(v)
