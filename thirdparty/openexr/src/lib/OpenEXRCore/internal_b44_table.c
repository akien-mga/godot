//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) DreamWorks Animation LLC and Contributors of the OpenEXR Project
//

#include <stdint.h>

extern uint16_t* exrcore_logTable;


static uint16_t exrcore_logTable_data[65536];
uint16_t* exrcore_logTable = exrcore_logTable_data;
