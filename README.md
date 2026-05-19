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
- [x] ldh a, [n16]
- [x] ldh [n16], a
- [x] ld a, [hl+]
- [x] ld [hl+], a
- [x] ld a, [hl-]
- [x] ld [hl-], a

### 16-bit load instructions

- [x] ld r16, n16
- [ ] ld [n16], sp
- [ ] ld sp, hl
- [ ] push r16
- [ ] pop r16
- [ ] ld hl, sp+e8

### 8-bit arithmetic and logical instructions

### 16-bit arithmetic instructions

### Control flow instructions

### Miscellaneous instructions