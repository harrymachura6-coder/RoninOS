#pragma once

#include <stdint.h>

void initrd_mount_from_multiboot(uint32_t mb_magic, uint32_t mb_info_addr);
