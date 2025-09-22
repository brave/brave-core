/*
 * Copyright (c) 2012-2020 MIRACL UK Ltd.
 *
 * This file is part of MIRACL Core
 * (see https://github.com/miracl/core).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
use crate::arch::Chunk;
use crate::bn254::big::NLEN;

// BN254 Modulus
// Base Bits= 56
pub const MODULUS: [Chunk; NLEN] = [0x13, 0x13A7, 0x80000000086121, 0x40000001BA344D, 0x25236482];
pub const ROI: [Chunk; NLEN] = [0x12, 0x13A7, 0x80000000086121, 0x40000001BA344D, 0x25236482];
pub const R2MODP: [Chunk; NLEN] = [
    0x2F2A96FF5E7E39,
    0x64E8642B96F13C,
    0x9926F7B00C7146,
    0x8321E7B4DACD24,
    0x1D127A2E,
];
pub const MCONST: Chunk = 0x435E50D79435E5;
pub const SQRTM3:[Chunk;NLEN]=[0x4,0x60C,0x3CF0F,0x4000000126CD89,0x25236482];
pub const FRA: [Chunk; NLEN] = [
    0x7DE6C06F2A6DE9,
    0x74924D3F77C2E1,
    0x50A846953F8509,
    0x212E7C8CB6499B,
    0x1B377619,
];
pub const FRB: [Chunk; NLEN] = [
    0x82193F90D5922A,
    0x8B6DB2C08850C5,
    0x2F57B96AC8DC17,
    0x1ED1837503EAB2,
    0x9EBEE69,
];

pub const CURVE_COF_I: isize = 1;
pub const CURVE_B_I: isize = 2;
pub const CURVE_B: [Chunk; NLEN] = [0x2, 0x0, 0x0, 0x0, 0x0];
pub const CURVE_ORDER: [Chunk; NLEN] = [
    0xD,
    0x800000000010A1,
    0x8000000007FF9F,
    0x40000001BA344D,
    0x25236482,
];
pub const CURVE_GX: [Chunk; NLEN] = [0x12, 0x13A7, 0x80000000086121, 0x40000001BA344D, 0x25236482];
pub const CURVE_GY: [Chunk; NLEN] = [0x1, 0x0, 0x0, 0x0, 0x0];
pub const CURVE_HTPC:[Chunk;NLEN]=[0x1,0x0,0x0,0x0,0x0];
pub const CURVE_BNX: [Chunk; NLEN] = [0x80000000000001, 0x40, 0x0, 0x0, 0x0];
pub const CURVE_COF: [Chunk; NLEN] = [0x1, 0x0, 0x0, 0x0, 0x0];
pub const CRU: [Chunk; NLEN] = [0x80000000000007, 0x6CD, 0x40000000024909, 0x49B362, 0x0];
pub const CURVE_PXA: [Chunk; NLEN] = [
    0xEE4224C803FB2B,
    0x8BBB4898BF0D91,
    0x7E8C61EDB6A464,
    0x519EB62FEB8D8C,
    0x61A10BB,
];
pub const CURVE_PXB: [Chunk; NLEN] = [
    0x8C34C1E7D54CF3,
    0x746BAE3784B70D,
    0x8C5982AA5B1F4D,
    0xBA737833310AA7,
    0x516AAF9,
];
pub const CURVE_PYA: [Chunk; NLEN] = [
    0xF0E07891CD2B9A,
    0xAE6BDBE09BD19,
    0x96698C822329BD,
    0x6BAF93439A90E0,
    0x21897A0,
];
pub const CURVE_PYB: [Chunk; NLEN] = [
    0x2D1AEC6B3ACE9B,
    0x6FFD739C9578A,
    0x56F5F38D37B090,
    0x7C8B15268F6D44,
    0xEBB2B0E,
];
pub const CURVE_W: [[Chunk; NLEN]; 2] = [
    [0x3, 0x80000000000204, 0x6181, 0x0, 0x0],
    [0x1, 0x81, 0x0, 0x0, 0x0],
];
pub const CURVE_SB: [[[Chunk; NLEN]; 2]; 2] = [
    [
        [0x4, 0x80000000000285, 0x6181, 0x0, 0x0],
        [0x1, 0x81, 0x0, 0x0, 0x0],
    ],
    [
        [0x1, 0x81, 0x0, 0x0, 0x0],
        [0xA, 0xE9D, 0x80000000079E1E, 0x40000001BA344D, 0x25236482],
    ],
];
pub const CURVE_WB: [[Chunk; NLEN]; 4] = [
    [0x80000000000000, 0x80000000000040, 0x2080, 0x0, 0x0],
    [0x80000000000005, 0x54A, 0x8000000001C707, 0x312241, 0x0],
    [
        0x80000000000003,
        0x800000000002C5,
        0xC000000000E383,
        0x189120,
        0x0,
    ],
    [0x80000000000001, 0x800000000000C1, 0x2080, 0x0, 0x0],
];
pub const CURVE_BB: [[[Chunk; NLEN]; 4]; 4] = [
    [
        [
            0x8000000000000D,
            0x80000000001060,
            0x8000000007FF9F,
            0x40000001BA344D,
            0x25236482,
        ],
        [
            0x8000000000000C,
            0x80000000001060,
            0x8000000007FF9F,
            0x40000001BA344D,
            0x25236482,
        ],
        [
            0x8000000000000C,
            0x80000000001060,
            0x8000000007FF9F,
            0x40000001BA344D,
            0x25236482,
        ],
        [0x2, 0x81, 0x0, 0x0, 0x0],
    ],
    [
        [0x1, 0x81, 0x0, 0x0, 0x0],
        [
            0x8000000000000C,
            0x80000000001060,
            0x8000000007FF9F,
            0x40000001BA344D,
            0x25236482,
        ],
        [
            0x8000000000000D,
            0x80000000001060,
            0x8000000007FF9F,
            0x40000001BA344D,
            0x25236482,
        ],
        [
            0x8000000000000C,
            0x80000000001060,
            0x8000000007FF9F,
            0x40000001BA344D,
            0x25236482,
        ],
    ],
    [
        [0x2, 0x81, 0x0, 0x0, 0x0],
        [0x1, 0x81, 0x0, 0x0, 0x0],
        [0x1, 0x81, 0x0, 0x0, 0x0],
        [0x1, 0x81, 0x0, 0x0, 0x0],
    ],
    [
        [0x80000000000002, 0x40, 0x0, 0x0, 0x0],
        [0x2, 0x102, 0x0, 0x0, 0x0],
        [
            0xA,
            0x80000000001020,
            0x8000000007FF9F,
            0x40000001BA344D,
            0x25236482,
        ],
        [0x80000000000002, 0x40, 0x0, 0x0, 0x0],
    ],
];

pub const USE_GLV: bool = true;
pub const USE_GS_G2: bool = true;
pub const USE_GS_GT: bool = true;
pub const GT_STRONG: bool = false;
