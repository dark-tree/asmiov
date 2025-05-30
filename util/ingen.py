#!/bin/env python3
# Usage: ingen.py <input>

import sys
input = sys.argv[1]

with open(input) as in_file:
	for line in in_file:
		try:
			start = line.index('INST ') + 5 + 4
			end = line.rindex('///')

			inst = line[start:end].rstrip()[:-1]
			open = inst.index('(')
			locs = inst.count('Location')
			name = inst[:open]

			print(f'if (argc == {locs} && strcmp(name, "{name}") == 0) return parseCall<{locs}>(reporter, &BufferWriter::put_{name}, writer, stream);')
		except:
			pass

