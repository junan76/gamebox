# gamebox

A cross platform gameboy emulator.

## CPU Instruction Set

### 8-bit load instructions

- [x] ld r8, r8
- [x] ld r8, n8
- [x] ld r8, [hl]
- [x] ld [hl], r8
- [x] ld [hl], n8
- [x] ld a, [r16]
- [x] ld [r16], a
- [x] ld a, [n16]
- [x] ld [n16], a
- [x] ldh a, [c]
- [x] ldh [c], a
- [x] ldh a, [n8]
- [x] ldh [n8], a
- [x] ld a, [hl-]
- [x] ld [hl-], a
- [x] ld a, [hl+]
- [x] ld [hl+], a

### 16-bit load instructions

- [x] ld r16, n16
- [x] ld [n16], sp
- [x] ld sp, hl
- [x] push r16
- [x] pop r16
- [x] ld hl, sp+e8

### 8-bit arithmetic and logical instructions
- [x] add a, r8
- [x] add a, [hl]
- [x] add a, n8
- [x] adc a, r8
- [x] adc a, [hl]
- [x] adc a, n8
- [x] sub a, r8
- [x] sub a, [hl]
- [x] sub a, n8
- [x] sbc a, r8
- [x] sbc a, [hl]
- [x] sbc a, n8

### 16-bit arithmetic instructions

### Control flow instructions
- [x] jp n16
- [x] jp hl
- [x] jp cc, n16
- [x] jr e8
- [x] jr cc, e8
- [x] call n16
- [x] call cc, n16
- [x] ret
- [x] ret cc
- [x] reti

### Miscellaneous instructions

- [x] halt
- [x] stop
- [x] di
- [x] ei
- [x] nop