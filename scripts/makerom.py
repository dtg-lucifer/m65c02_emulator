# Define ROM size constants
ROM_SIZE = 32768  # 32K
ROM_BASE = 0x8000  # ROM starts at 0x8000
RESET_VECTOR = 0xFFFC  # Reset vector address

# Example program code (will be placed at 0x8000)
code = bytearray([
  0xa9, 0xff,         # lda #$ff
  0x8d, 0x02, 0x60,   # sta $6002

  0xa9, 0x55,         # lda #$55
  0x8d, 0x00, 0x60,   # sta $6000

  0xa9, 0xaa,         # lda #$aa
  0x8d, 0x00, 0x60,   # sta $6000

  0x4c, 0x05, 0x80,   # jmp $8005
])

# Create empty ROM filled with NOPs (0xEA)
rom = bytearray([0xea] * ROM_SIZE)

# Place code at beginning of ROM (at offset 0 from ROM base)
for i in range(len(code)):
    rom[i] = code[i]

# Set reset vector to point to ROM base (0x8000)
# Calculate offset for reset vector in ROM: 0xFFFC - 0x8000 = 0x7FFC
reset_vector_offset = RESET_VECTOR - ROM_BASE
rom[reset_vector_offset] = ROM_BASE & 0xFF        # Low byte
rom[reset_vector_offset + 1] = (ROM_BASE >> 8) & 0xFF  # High byte

print(f"Reset vector at 0x{RESET_VECTOR:04X} (offset 0x{reset_vector_offset:04X}) set to 0x{ROM_BASE:04X}")

# Write ROM to file
with open("rom.bin", "wb") as out_file:
  out_file.write(rom)
  print(f"ROM file written ({ROM_SIZE} bytes)")
