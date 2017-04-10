#!/usr/bin/python

import os
import sys
import tempfile


input_files = []
output_file = ""
is_script = False
symbol="data"

source_map = {}

i = 1
while i < len(sys.argv):
    if sys.argv[i] == "--script":
        is_script = True
    elif sys.argv[i] == "--output":
        output_file = sys.argv[i+1]
        i = i + 1
    elif sys.argv[i] == "--symbol":
        symbol = sys.argv[i+1]
        i = i + 1
    else:
        input_files.append(sys.argv[i])
    i = i + 1


if is_script:
    luac_out = tempfile.mktemp()

    if os.system("luac -o %s %s" % (luac_out, " ".join(input_files))) == 0:
        f = open(luac_out, "r")
        try:
            source_data = f.read()
        finally:
            f.close()
    os.unlink(luac_out)

    if not source_data:
        source_data = ""
        for src_file in input_files:
            f = open(src_file, "rt")
            try:
                source_data = source_data + "\n" + f.read()
            finally:
                f.close()

    c_source_data = ""

    idx = 0
    for i in range(0, len(source_data)):
        c = source_data[i:i+1]
        c_source_data = c_source_data + hex(ord(c)) + ","
        idx = idx + 1
        if idx % 20 == 0:
            c_source_data = c_source_data + "\n"

else:
    c_source_data = ""
    
    pos = 0
    for src_file in input_files:
        f = open(src_file, "rt");
        try:
            idx = 0
            source_data = f.read();
            source_map[os.path.basename(src_file)] = (pos, len(source_data))
            pos = pos + len(source_data)
            for i in range(0, len(source_data)):
                c = source_data[i:i+1]
                c_source_data = c_source_data + hex(ord(c)) + ","
                idx = idx + 1
                if idx % 20 == 0:
                    c_source_data = c_source_data + "\n"
        finally:
            f.close()


fout = open(output_file, "wt")

fout.write("""
/****************************************************
 * generate by gen-scripts.py
 * %s
 *
 *
 *
 ****************************************************/
"""% " ".join(sys.argv))

fout.write("""
static const char %(symbol)s_data [] = {
%(source_data)s
};
static const int %(symbol)s_data_len = sizeof(%(symbol)s_data);

""" % {"symbol":symbol, "source_data":c_source_data})

if not is_script:
    fout.write("""
static const struct {
    const char* file_name;
    int start;
    int length;
} %s_map_file[] = {
"""%symbol)
    for file_name, pos in source_map.items():
        fout.write("\t{\"%s\", %d, %d},\n" % (file_name, pos[0], pos[1]))

    fout.write("""};
static const int %(symbol)s_map_file_count =
    sizeof(%(symbol)s_map_file) / sizeof(%(symbol)s_map_file[0]);

"""%{"symbol":symbol});

fout.close()
