# Purpose of this Python script:
#
#    Shrink a compiled CSGO map file (".bsp") down to only the data that is
#    currently parsed and used by DZSimulator. All unused parts are deleted to
#    reduce the map's file size. This script is used to generate a small CSGO
#    map file that can be embedded into DZSimulator's executable.
#    (Before running this script, you should remove unnecessary packed files
#    from the BSP file to further reduce file size)
#
# CAUTION:
#    The file is modified in place. Backup the file before using this script.
# 
# NOTE:
#    - Any ".bsp" file that was processed by this script cannot be opened by CSGO.
#    - Any ".bsp" file that was processed by this script cannot be used by BSP
#      packing tools such as VIDE's Pakfile Lump editor.
#      -> Pack/unpack the BSP file BEFORE processing it with this script!
#         - CAUTION: Manually deleting packed files from a BSP with VIDE often
#           corrupted the BSP's packed files for me!
#           I avoided that by first deleting ALL packed files, saving the BSP,
#           then packing all required files and saving the BSP again.

# Make sure this is a complete list of all lumps that the current version of
# DZSimulator makes use of!
LUMPS_USED_BY_DZSIMULATOR = [ # Lumps used by DZSimulator version 0.0.4
     0, # LUMP_IDX_ENTITIES
     1, # LUMP_IDX_PLANES
     2, # LUMP_IDX_TEXDATA
     3, # LUMP_IDX_VERTEXES
     5, # LUMP_IDX_NODES
     6, # LUMP_IDX_TEXINFO
     7, # LUMP_IDX_FACES
    10, # LUMP_IDX_LEAFS
    12, # LUMP_IDX_EDGES
    13, # LUMP_IDX_SURFEDGES
    14, # LUMP_IDX_MODELS
    17, # LUMP_IDX_LEAFBRUSHES
    18, # LUMP_IDX_BRUSHES
    19, # LUMP_IDX_BRUSHSIDES
    26, # LUMP_IDX_DISPINFO
    33, # LUMP_IDX_DISP_VERTS
    35, # LUMP_IDX_GAME_LUMP (only static props are used from this)
    40, # LUMP_IDX_PAKFILE
    43, # LUMP_IDX_TEXDATA_STRING_DATA
    44, # LUMP_IDX_TEXDATA_STRING_TABLE
]

# File format: https://developer.valvesoftware.com/wiki/Source_BSP_File_Format

import sys
import struct

# Check if filename was provided as command-line argument
if len(sys.argv) < 2:
    print("Please provide a filename as a command-line argument.")
    sys.exit(1)

filename = sys.argv[1]

# Read file in bytes
orig_file_contents = []
with open(filename, "rb") as f:
    orig_file_contents = f.read()

# Parse lump directory (assuming little-endianness)
LUMP_DIR_FILE_OFS = 8
LUMP_DIR_ENTRIES = 64
LUMP_DIR_ENTRY_SIZE = 16
lump_dir_bytes = orig_file_contents[LUMP_DIR_FILE_OFS:LUMP_DIR_FILE_OFS+(LUMP_DIR_ENTRIES*LUMP_DIR_ENTRY_SIZE)]
num_ints = LUMP_DIR_ENTRIES * (LUMP_DIR_ENTRY_SIZE // 4)
lump_dir_ints = struct.unpack("<" + "i" * num_ints, lump_dir_bytes)

# Get contents of required lumps
new_data_per_lump = [b''] * 64
for lump_idx in LUMPS_USED_BY_DZSIMULATOR:
    lump_data_start_pos = lump_dir_ints[(lump_idx * 4) + 0]
    lump_data_len       = lump_dir_ints[(lump_idx * 4) + 1]
    lump_data = orig_file_contents[lump_data_start_pos:lump_data_start_pos+lump_data_len]

    # Special case where lump data itself must be modified
    if lump_idx == 35: # LUMP_IDX_GAME_LUMP
        sprp_lump_data = []
        sprp_lump_flagsversion_field = None
        gamelump_count = struct.unpack("<i", lump_data[0:4])[0]
        gamelump_dir_bytes = lump_data[4:4+(gamelump_count * 16)]
        gamelump_dir_ints = struct.unpack("<" + "i" * (gamelump_count * 4), gamelump_dir_bytes)
        for gamelump_idx in range(gamelump_count):
            gamelump_id             = gamelump_dir_ints[(gamelump_idx * 4) + 0]
            gamelump_flagsversion   = gamelump_dir_ints[(gamelump_idx * 4) + 1]
            gamelump_data_start_pos = gamelump_dir_ints[(gamelump_idx * 4) + 2]
            gamelump_data_len       = gamelump_dir_ints[(gamelump_idx * 4) + 3]
            if gamelump_id == 1936749168: # value of ASCII 'sprp'
                sprp_lump_data = orig_file_contents[gamelump_data_start_pos:gamelump_data_start_pos+gamelump_data_len]
                sprp_lump_flagsversion_field = gamelump_flagsversion
                break
        if len(sprp_lump_data) == 0:
            continue # Don't include the game lump if there are no static props
        # Modify lump data to only contain static props
        orig_lump_data = lump_data
        game_lump_header_ints = [
            1, # game lump count. Only sprp lump is present.
            1936749168,                   # sprp lump ID
            sprp_lump_flagsversion_field, # sprp lump version and flags
            0,                            # sprp lump file offset (gets set later)
            len(sprp_lump_data),          # sprp lump length
        ]
        lump_data = struct.pack('<{}L'.format(5), *game_lump_header_ints)
        lump_data += sprp_lump_data

    new_data_per_lump[lump_idx] = lump_data

# Generate new lump directory
new_lump_dir_ints = [0] * (64 * 4)
current_write_pos = 1040
for lump_idx in LUMPS_USED_BY_DZSIMULATOR:
    new_lump_data_start_pos = current_write_pos
    new_lump_data_len       = len(new_data_per_lump[lump_idx])
    new_lump_version        = lump_dir_ints[(lump_idx * 4) + 2]
    new_lump_dir_ints[(lump_idx * 4) + 0] = new_lump_data_start_pos
    new_lump_dir_ints[(lump_idx * 4) + 1] = new_lump_data_len
    new_lump_dir_ints[(lump_idx * 4) + 2] = new_lump_version

    current_write_pos += new_lump_data_len
    if new_lump_data_len % 16 != 0: # Fill with zeros until multiple of 16
        current_write_pos += 16 - (new_lump_data_len % 16)

# Special case: Update file offset value within the game lump
if len(new_data_per_lump[35]) != 0: # If there are any static props
    sprp_lump_data_start_pos = new_lump_dir_ints[(35 * 4) + 0] + 20
    sprp_lump_fileofs_bytes = struct.pack('<{}L'.format(1), *[sprp_lump_data_start_pos])
    new_data_per_lump[35] = new_data_per_lump[35][:12] + sprp_lump_fileofs_bytes + new_data_per_lump[35][12+4:]

# Generate new file contents
new_file_contents = orig_file_contents[:8] # Copy beginning of header 
new_file_contents += struct.pack('<{}L'.format(64 * 4), *new_lump_dir_ints) # Add new lump dir
new_file_contents += orig_file_contents[1032:1032+4] # Copy end of header 
new_file_contents += b'\x00' * 4 # Fill up with 4 zeros
for lump_idx in LUMPS_USED_BY_DZSIMULATOR:
    new_file_contents += new_data_per_lump[lump_idx]
    lump_data_len = len(new_data_per_lump[lump_idx])
    if lump_data_len % 16 != 0: # Fill with zeros until multiple of 16
        new_file_contents += b'\x00' * (16 - (lump_data_len % 16))

# Save changes to file
with open(filename, 'wb') as f:
    f.write(new_file_contents)

print("DONE with shrinking CSGO map file.")
