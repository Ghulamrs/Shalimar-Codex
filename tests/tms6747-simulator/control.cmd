--stack_size=0x1000
--heap_size=0x1000
MEMORY { L2RAM : origin = 0x11800000, length = 0x00040000 }
SECTIONS {
    .text > L2RAM
    .stack > L2RAM
    .bss > L2RAM
    .neardata > L2RAM
    .rodata > L2RAM
    .data > L2RAM
    .const > L2RAM
    .cinit > L2RAM
    .pinit > L2RAM
    .init_array > L2RAM
    .far > L2RAM
    .fardata > L2RAM
    .cio > L2RAM
    .sysmem > L2RAM
    .switch > L2RAM
}
