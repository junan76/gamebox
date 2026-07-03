# gamebox

A cross platform gameboy emulator.

## The Design

### Application layer

### Core emulator layer

1. cpu(DONE)
2. ppu(DOING)
3. apu(TODO)
5. bus(DONE)
6. devices
   - joypad(DONE)
   - serial(DONE)
   - timer(DONE)
7. mbc(DOING)


### HAL layer

1. fs
   - hal_open
   - hal_close
   - hal_load
2. input
   - hal_input_poll
3. graphics
   - hal_draw_line
4. audio
   - hal_audio_play
5. time
   - hal_delay
6. init/deinit
   - hal_init
   - hal_exit

### Drivers layer

### Ports layer

### MCU specific SDK layer

---

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
- [x] cp a, r8
- [x] cp a, [hl]
- [x] cp a, n8
- [x] inc r8
- [x] inc [hl]
- [x] dec r8
- [x] dec [hl]
- [x] and a, r8
- [x] and a, [hl]
- [x] and a, n8
- [x] or a, r8
- [x] or a, [hl]
- [x] or a, n8
- [x] xor a, r8
- [x] xor a, [hl]
- [x] xor a, n8
- [x] ccf
- [x] scf
- [x] daa
- [x] cpl

### 16-bit arithmetic instructions
- [x] inc r16
- [x] dec r16
- [x] add hl, r16
- [x] add sp, e8

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