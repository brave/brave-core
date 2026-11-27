//! CRC algorithms as structs.
use crate::Algorithm;

/// # [`CRC-3/GSM`][1]
///
/// - `width`: `3` bits
/// - `poly`: `0x3` (reversed: `0x6`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x7`
/// - `check`: `0x4`
/// - `residue`: `0x2`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-3-gsm
pub const CRC_3_GSM: Algorithm<u8> = Algorithm {
    width: 3,
    poly: 0x3,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x7,
    check: 0x4,
    residue: 0x2
};

/// # [`CRC-3/ROHC`][1]
///
/// - `width`: `3` bits
/// - `poly`: `0x3` (reversed: `0x6`)
/// - `init`: `0x7`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x6`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-3-rohc
pub const CRC_3_ROHC: Algorithm<u8> = Algorithm {
    width: 3,
    poly: 0x3,
    init: 0x7,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x6,
    residue: 0x0
};

/// # [`CRC-4/G-704`][1]
///
/// - `width`: `4` bits
/// - `poly`: `0x3` (reversed: `0xc`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x7`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-4-g-704
pub const CRC_4_G_704: Algorithm<u8> = Algorithm {
    width: 4,
    poly: 0x3,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x7,
    residue: 0x0
};

/// # [`CRC-4/INTERLAKEN`][1]
///
/// - `width`: `4` bits
/// - `poly`: `0x3` (reversed: `0xc`)
/// - `init`: `0xf`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xf`
/// - `check`: `0xb`
/// - `residue`: `0x2`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-4-interlaken
pub const CRC_4_INTERLAKEN: Algorithm<u8> = Algorithm {
    width: 4,
    poly: 0x3,
    init: 0xf,
    refin: false,
    refout: false,
    xorout: 0xf,
    check: 0xb,
    residue: 0x2
};

/// # [`CRC-5/EPC-C1G2`][1]
///
/// - `width`: `5` bits
/// - `poly`: `0x9` (reversed: `0x12`)
/// - `init`: `0x9`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x0`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-5-epc-c1g2
pub const CRC_5_EPC_C1G2: Algorithm<u8> = Algorithm {
    width: 5,
    poly: 0x9,
    init: 0x9,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x0,
    residue: 0x0
};

/// # [`CRC-5/G-704`][1]
///
/// - `width`: `5` bits
/// - `poly`: `0x15` (reversed: `0x15`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x7`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-5-g-704
pub const CRC_5_G_704: Algorithm<u8> = Algorithm {
    width: 5,
    poly: 0x15,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x7,
    residue: 0x0
};

/// # [`CRC-5/USB`][1]
///
/// - `width`: `5` bits
/// - `poly`: `0x5` (reversed: `0x14`)
/// - `init`: `0x1f`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x1f`
/// - `check`: `0x19`
/// - `residue`: `0x6`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-5-usb
pub const CRC_5_USB: Algorithm<u8> = Algorithm {
    width: 5,
    poly: 0x5,
    init: 0x1f,
    refin: true,
    refout: true,
    xorout: 0x1f,
    check: 0x19,
    residue: 0x6
};

/// # [`CRC-6/CDMA2000-A`][1]
///
/// - `width`: `6` bits
/// - `poly`: `0x27` (reversed: `0x39`)
/// - `init`: `0x3f`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xd`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-6-cdma2000-a
pub const CRC_6_CDMA2000_A: Algorithm<u8> = Algorithm {
    width: 6,
    poly: 0x27,
    init: 0x3f,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xd,
    residue: 0x0
};

/// # [`CRC-6/CDMA2000-B`][1]
///
/// - `width`: `6` bits
/// - `poly`: `0x7` (reversed: `0x38`)
/// - `init`: `0x3f`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x3b`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-6-cdma2000-b
pub const CRC_6_CDMA2000_B: Algorithm<u8> = Algorithm {
    width: 6,
    poly: 0x7,
    init: 0x3f,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x3b,
    residue: 0x0
};

/// # [`CRC-6/DARC`][1]
///
/// - `width`: `6` bits
/// - `poly`: `0x19` (reversed: `0x26`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x26`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-6-darc
pub const CRC_6_DARC: Algorithm<u8> = Algorithm {
    width: 6,
    poly: 0x19,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x26,
    residue: 0x0
};

/// # [`CRC-6/G-704`][1]
///
/// - `width`: `6` bits
/// - `poly`: `0x3` (reversed: `0x30`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x6`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-6-g-704
pub const CRC_6_G_704: Algorithm<u8> = Algorithm {
    width: 6,
    poly: 0x3,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x6,
    residue: 0x0
};

/// # [`CRC-6/GSM`][1]
///
/// - `width`: `6` bits
/// - `poly`: `0x2f` (reversed: `0x3d`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x3f`
/// - `check`: `0x13`
/// - `residue`: `0x3a`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-6-gsm
pub const CRC_6_GSM: Algorithm<u8> = Algorithm {
    width: 6,
    poly: 0x2f,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x3f,
    check: 0x13,
    residue: 0x3a
};

/// # [`CRC-7/MMC`][1]
///
/// - `width`: `7` bits
/// - `poly`: `0x9` (reversed: `0x48`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x75`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-7-mmc
pub const CRC_7_MMC: Algorithm<u8> = Algorithm {
    width: 7,
    poly: 0x9,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x75,
    residue: 0x0
};

/// # [`CRC-7/ROHC`][1]
///
/// - `width`: `7` bits
/// - `poly`: `0x4f` (reversed: `0x79`)
/// - `init`: `0x7f`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x53`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-7-rohc
pub const CRC_7_ROHC: Algorithm<u8> = Algorithm {
    width: 7,
    poly: 0x4f,
    init: 0x7f,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x53,
    residue: 0x0
};

/// # [`CRC-7/UMTS`][1]
///
/// - `width`: `7` bits
/// - `poly`: `0x45` (reversed: `0x51`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x61`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-7-umts
pub const CRC_7_UMTS: Algorithm<u8> = Algorithm {
    width: 7,
    poly: 0x45,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x61,
    residue: 0x0
};

/// # [`CRC-8/AUTOSAR`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x2f` (reversed: `0xf4`)
/// - `init`: `0xff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xff`
/// - `check`: `0xdf`
/// - `residue`: `0x42`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-autosar
pub const CRC_8_AUTOSAR: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x2f,
    init: 0xff,
    refin: false,
    refout: false,
    xorout: 0xff,
    check: 0xdf,
    residue: 0x42
};

/// # [`CRC-8/BLUETOOTH`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0xa7` (reversed: `0xe5`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x26`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-bluetooth
pub const CRC_8_BLUETOOTH: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0xa7,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x26,
    residue: 0x0
};

/// # [`CRC-8/CDMA2000`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x9b` (reversed: `0xd9`)
/// - `init`: `0xff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xda`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-cdma2000
pub const CRC_8_CDMA2000: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x9b,
    init: 0xff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xda,
    residue: 0x0
};

/// # [`CRC-8/DARC`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x39` (reversed: `0x9c`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x15`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-darc
pub const CRC_8_DARC: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x39,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x15,
    residue: 0x0
};

/// # [`CRC-8/DVB-S2`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0xd5` (reversed: `0xab`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xbc`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-dvb-s2
pub const CRC_8_DVB_S2: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0xd5,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xbc,
    residue: 0x0
};

/// # [`CRC-8/GSM-A`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x1d` (reversed: `0xb8`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x37`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-gsm-a
pub const CRC_8_GSM_A: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x1d,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x37,
    residue: 0x0
};

/// # [`CRC-8/GSM-B`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x49` (reversed: `0x92`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xff`
/// - `check`: `0x94`
/// - `residue`: `0x53`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-gsm-b
pub const CRC_8_GSM_B: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x49,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0xff,
    check: 0x94,
    residue: 0x53
};

/// # [`CRC-8/HITAG`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x1d` (reversed: `0xb8`)
/// - `init`: `0xff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xb4`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-hitag
pub const CRC_8_HITAG: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x1d,
    init: 0xff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xb4,
    residue: 0x0
};

/// # [`CRC-8/I-432-1`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x7` (reversed: `0xe0`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x55`
/// - `check`: `0xa1`
/// - `residue`: `0xac`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-i-432-1
pub const CRC_8_I_432_1: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x7,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x55,
    check: 0xa1,
    residue: 0xac
};

/// # [`CRC-8/I-CODE`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x1d` (reversed: `0xb8`)
/// - `init`: `0xfd`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x7e`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-i-code
pub const CRC_8_I_CODE: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x1d,
    init: 0xfd,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x7e,
    residue: 0x0
};

/// # [`CRC-8/LTE`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x9b` (reversed: `0xd9`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xea`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-lte
pub const CRC_8_LTE: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x9b,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xea,
    residue: 0x0
};

/// # [`CRC-8/MAXIM-DOW`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x31` (reversed: `0x8c`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xa1`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-maxim-dow
pub const CRC_8_MAXIM_DOW: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x31,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xa1,
    residue: 0x0
};

/// # [`CRC-8/MIFARE-MAD`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x1d` (reversed: `0xb8`)
/// - `init`: `0xc7`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x99`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-mifare-mad
pub const CRC_8_MIFARE_MAD: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x1d,
    init: 0xc7,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x99,
    residue: 0x0
};

/// # [`CRC-8/NRSC-5`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x31` (reversed: `0x8c`)
/// - `init`: `0xff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xf7`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-nrsc-5
pub const CRC_8_NRSC_5: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x31,
    init: 0xff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xf7,
    residue: 0x0
};

/// # [`CRC-8/OPENSAFETY`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x2f` (reversed: `0xf4`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x3e`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-opensafety
pub const CRC_8_OPENSAFETY: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x2f,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x3e,
    residue: 0x0
};

/// # [`CRC-8/ROHC`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x7` (reversed: `0xe0`)
/// - `init`: `0xff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xd0`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-rohc
pub const CRC_8_ROHC: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x7,
    init: 0xff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xd0,
    residue: 0x0
};

/// # [`CRC-8/SAE-J1850`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x1d` (reversed: `0xb8`)
/// - `init`: `0xff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xff`
/// - `check`: `0x4b`
/// - `residue`: `0xc4`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-sae-j1850
pub const CRC_8_SAE_J1850: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x1d,
    init: 0xff,
    refin: false,
    refout: false,
    xorout: 0xff,
    check: 0x4b,
    residue: 0xc4
};

/// # [`CRC-8/SMBUS`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x7` (reversed: `0xe0`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xf4`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-smbus
pub const CRC_8_SMBUS: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x7,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xf4,
    residue: 0x0
};

/// # [`CRC-8/TECH-3250`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x1d` (reversed: `0xb8`)
/// - `init`: `0xff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x97`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-tech-3250
pub const CRC_8_TECH_3250: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x1d,
    init: 0xff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x97,
    residue: 0x0
};

/// # [`CRC-8/WCDMA`][1]
///
/// - `width`: `8` bits
/// - `poly`: `0x9b` (reversed: `0xd9`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x25`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-8-wcdma
pub const CRC_8_WCDMA: Algorithm<u8> = Algorithm {
    width: 8,
    poly: 0x9b,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x25,
    residue: 0x0
};

/// # [`CRC-10/ATM`][1]
///
/// - `width`: `10` bits
/// - `poly`: `0x233` (reversed: `0x331`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x199`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-10-atm
pub const CRC_10_ATM: Algorithm<u16> = Algorithm {
    width: 10,
    poly: 0x233,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x199,
    residue: 0x0
};

/// # [`CRC-10/CDMA2000`][1]
///
/// - `width`: `10` bits
/// - `poly`: `0x3d9` (reversed: `0x26f`)
/// - `init`: `0x3ff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x233`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-10-cdma2000
pub const CRC_10_CDMA2000: Algorithm<u16> = Algorithm {
    width: 10,
    poly: 0x3d9,
    init: 0x3ff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x233,
    residue: 0x0
};

/// # [`CRC-10/GSM`][1]
///
/// - `width`: `10` bits
/// - `poly`: `0x175` (reversed: `0x2ba`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x3ff`
/// - `check`: `0x12a`
/// - `residue`: `0xc6`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-10-gsm
pub const CRC_10_GSM: Algorithm<u16> = Algorithm {
    width: 10,
    poly: 0x175,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x3ff,
    check: 0x12a,
    residue: 0xc6
};

/// # [`CRC-11/FLEXRAY`][1]
///
/// - `width`: `11` bits
/// - `poly`: `0x385` (reversed: `0x50e`)
/// - `init`: `0x1a`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x5a3`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-11-flexray
pub const CRC_11_FLEXRAY: Algorithm<u16> = Algorithm {
    width: 11,
    poly: 0x385,
    init: 0x1a,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x5a3,
    residue: 0x0
};

/// # [`CRC-11/UMTS`][1]
///
/// - `width`: `11` bits
/// - `poly`: `0x307` (reversed: `0x706`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x61`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-11-umts
pub const CRC_11_UMTS: Algorithm<u16> = Algorithm {
    width: 11,
    poly: 0x307,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x61,
    residue: 0x0
};

/// # [`CRC-12/CDMA2000`][1]
///
/// - `width`: `12` bits
/// - `poly`: `0xf13` (reversed: `0xc8f`)
/// - `init`: `0xfff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xd4d`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-12-cdma2000
pub const CRC_12_CDMA2000: Algorithm<u16> = Algorithm {
    width: 12,
    poly: 0xf13,
    init: 0xfff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xd4d,
    residue: 0x0
};

/// # [`CRC-12/DECT`][1]
///
/// - `width`: `12` bits
/// - `poly`: `0x80f` (reversed: `0xf01`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xf5b`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-12-dect
pub const CRC_12_DECT: Algorithm<u16> = Algorithm {
    width: 12,
    poly: 0x80f,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xf5b,
    residue: 0x0
};

/// # [`CRC-12/GSM`][1]
///
/// - `width`: `12` bits
/// - `poly`: `0xd31` (reversed: `0x8cb`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xfff`
/// - `check`: `0xb34`
/// - `residue`: `0x178`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-12-gsm
pub const CRC_12_GSM: Algorithm<u16> = Algorithm {
    width: 12,
    poly: 0xd31,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0xfff,
    check: 0xb34,
    residue: 0x178
};

/// # [`CRC-12/UMTS`][1]
///
/// - `width`: `12` bits
/// - `poly`: `0x80f` (reversed: `0xf01`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xdaf`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-12-umts
pub const CRC_12_UMTS: Algorithm<u16> = Algorithm {
    width: 12,
    poly: 0x80f,
    init: 0x0,
    refin: false,
    refout: true,
    xorout: 0x0,
    check: 0xdaf,
    residue: 0x0
};

/// # [`CRC-13/BBC`][1]
///
/// - `width`: `13` bits
/// - `poly`: `0x1cf5` (reversed: `0x15e7`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x4fa`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-13-bbc
pub const CRC_13_BBC: Algorithm<u16> = Algorithm {
    width: 13,
    poly: 0x1cf5,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x4fa,
    residue: 0x0
};

/// # [`CRC-14/DARC`][1]
///
/// - `width`: `14` bits
/// - `poly`: `0x805` (reversed: `0x2804`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x82d`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-14-darc
pub const CRC_14_DARC: Algorithm<u16> = Algorithm {
    width: 14,
    poly: 0x805,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x82d,
    residue: 0x0
};

/// # [`CRC-14/GSM`][1]
///
/// - `width`: `14` bits
/// - `poly`: `0x202d` (reversed: `0x2d01`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x3fff`
/// - `check`: `0x30ae`
/// - `residue`: `0x31e`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-14-gsm
pub const CRC_14_GSM: Algorithm<u16> = Algorithm {
    width: 14,
    poly: 0x202d,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x3fff,
    check: 0x30ae,
    residue: 0x31e
};

/// # [`CRC-15/CAN`][1]
///
/// - `width`: `15` bits
/// - `poly`: `0x4599` (reversed: `0x4cd1`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x59e`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-15-can
pub const CRC_15_CAN: Algorithm<u16> = Algorithm {
    width: 15,
    poly: 0x4599,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x59e,
    residue: 0x0
};

/// # [`CRC-15/MPT1327`][1]
///
/// - `width`: `15` bits
/// - `poly`: `0x6815` (reversed: `0x540b`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x1`
/// - `check`: `0x2566`
/// - `residue`: `0x6815`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-15-mpt1327
pub const CRC_15_MPT1327: Algorithm<u16> = Algorithm {
    width: 15,
    poly: 0x6815,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x1,
    check: 0x2566,
    residue: 0x6815
};

/// # [`CRC-16/ARC`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8005` (reversed: `0xa001`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xbb3d`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-arc
pub const CRC_16_ARC: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8005,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xbb3d,
    residue: 0x0
};

/// # [`CRC-16/CDMA2000`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0xc867` (reversed: `0xe613`)
/// - `init`: `0xffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x4c06`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-cdma2000
pub const CRC_16_CDMA2000: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0xc867,
    init: 0xffff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x4c06,
    residue: 0x0
};

/// # [`CRC-16/CMS`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8005` (reversed: `0xa001`)
/// - `init`: `0xffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xaee7`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-cms
pub const CRC_16_CMS: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8005,
    init: 0xffff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xaee7,
    residue: 0x0
};

/// # [`CRC-16/DDS-110`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8005` (reversed: `0xa001`)
/// - `init`: `0x800d`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x9ecf`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-dds-110
pub const CRC_16_DDS_110: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8005,
    init: 0x800d,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x9ecf,
    residue: 0x0
};

/// # [`CRC-16/DECT-R`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x589` (reversed: `0x91a0`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x1`
/// - `check`: `0x7e`
/// - `residue`: `0x589`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-dect-r
pub const CRC_16_DECT_R: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x589,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x1,
    check: 0x7e,
    residue: 0x589
};

/// # [`CRC-16/DECT-X`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x589` (reversed: `0x91a0`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x7f`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-dect-x
pub const CRC_16_DECT_X: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x589,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x7f,
    residue: 0x0
};

/// # [`CRC-16/DNP`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x3d65` (reversed: `0xa6bc`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffff`
/// - `check`: `0xea82`
/// - `residue`: `0x66c5`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-dnp
pub const CRC_16_DNP: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x3d65,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0xffff,
    check: 0xea82,
    residue: 0x66c5
};

/// # [`CRC-16/EN-13757`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x3d65` (reversed: `0xa6bc`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffff`
/// - `check`: `0xc2b7`
/// - `residue`: `0xa366`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-en-13757
pub const CRC_16_EN_13757: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x3d65,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0xffff,
    check: 0xc2b7,
    residue: 0xa366
};

/// # [`CRC-16/GENIBUS`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0xffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffff`
/// - `check`: `0xd64e`
/// - `residue`: `0x1d0f`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-genibus
pub const CRC_16_GENIBUS: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0xffff,
    refin: false,
    refout: false,
    xorout: 0xffff,
    check: 0xd64e,
    residue: 0x1d0f
};

/// # [`CRC-16/GSM`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffff`
/// - `check`: `0xce3c`
/// - `residue`: `0x1d0f`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-gsm
pub const CRC_16_GSM: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0xffff,
    check: 0xce3c,
    residue: 0x1d0f
};

/// # [`CRC-16/IBM-3740`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0xffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x29b1`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-ibm-3740
pub const CRC_16_IBM_3740: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0xffff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x29b1,
    residue: 0x0
};

/// # [`CRC-16/IBM-SDLC`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0xffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffff`
/// - `check`: `0x906e`
/// - `residue`: `0xf0b8`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-ibm-sdlc
pub const CRC_16_IBM_SDLC: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0xffff,
    refin: true,
    refout: true,
    xorout: 0xffff,
    check: 0x906e,
    residue: 0xf0b8
};

/// # [`CRC-16/ISO-IEC-14443-3-A`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0xc6c6`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xbf05`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-iso-iec-14443-3-a
pub const CRC_16_ISO_IEC_14443_3_A: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0xc6c6,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xbf05,
    residue: 0x0
};

/// # [`CRC-16/KERMIT`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x2189`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-kermit
pub const CRC_16_KERMIT: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x2189,
    residue: 0x0
};

/// # [`CRC-16/LJ1200`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x6f63` (reversed: `0xc6f6`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xbdf4`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-lj1200
pub const CRC_16_LJ1200: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x6f63,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xbdf4,
    residue: 0x0
};

/// # [`CRC-16/M17`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x5935` (reversed: `0xac9a`)
/// - `init`: `0xffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x772b`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-m17
pub const CRC_16_M17: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x5935,
    init: 0xffff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x772b,
    residue: 0x0
};

/// # [`CRC-16/MAXIM-DOW`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8005` (reversed: `0xa001`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffff`
/// - `check`: `0x44c2`
/// - `residue`: `0xb001`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-maxim-dow
pub const CRC_16_MAXIM_DOW: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8005,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0xffff,
    check: 0x44c2,
    residue: 0xb001
};

/// # [`CRC-16/MCRF4XX`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0xffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x6f91`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-mcrf4xx
pub const CRC_16_MCRF4XX: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0xffff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x6f91,
    residue: 0x0
};

/// # [`CRC-16/MODBUS`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8005` (reversed: `0xa001`)
/// - `init`: `0xffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x4b37`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-modbus
pub const CRC_16_MODBUS: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8005,
    init: 0xffff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x4b37,
    residue: 0x0
};

/// # [`CRC-16/NRSC-5`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x80b` (reversed: `0xd010`)
/// - `init`: `0xffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xa066`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-nrsc-5
pub const CRC_16_NRSC_5: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x80b,
    init: 0xffff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xa066,
    residue: 0x0
};

/// # [`CRC-16/OPENSAFETY-A`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x5935` (reversed: `0xac9a`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x5d38`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-opensafety-a
pub const CRC_16_OPENSAFETY_A: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x5935,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x5d38,
    residue: 0x0
};

/// # [`CRC-16/OPENSAFETY-B`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x755b` (reversed: `0xdaae`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x20fe`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-opensafety-b
pub const CRC_16_OPENSAFETY_B: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x755b,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x20fe,
    residue: 0x0
};

/// # [`CRC-16/PROFIBUS`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1dcf` (reversed: `0xf3b8`)
/// - `init`: `0xffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffff`
/// - `check`: `0xa819`
/// - `residue`: `0xe394`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-profibus
pub const CRC_16_PROFIBUS: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1dcf,
    init: 0xffff,
    refin: false,
    refout: false,
    xorout: 0xffff,
    check: 0xa819,
    residue: 0xe394
};

/// # [`CRC-16/RIELLO`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0xb2aa`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x63d0`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-riello
pub const CRC_16_RIELLO: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0xb2aa,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x63d0,
    residue: 0x0
};

/// # [`CRC-16/SPI-FUJITSU`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0x1d0f`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xe5cc`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-spi-fujitsu
pub const CRC_16_SPI_FUJITSU: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0x1d0f,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xe5cc,
    residue: 0x0
};

/// # [`CRC-16/T10-DIF`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8bb7` (reversed: `0xedd1`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xd0db`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-t10-dif
pub const CRC_16_T10_DIF: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8bb7,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xd0db,
    residue: 0x0
};

/// # [`CRC-16/TELEDISK`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0xa097` (reversed: `0xe905`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xfb3`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-teledisk
pub const CRC_16_TELEDISK: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0xa097,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xfb3,
    residue: 0x0
};

/// # [`CRC-16/TMS37157`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0x89ec`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x26b1`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-tms37157
pub const CRC_16_TMS37157: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0x89ec,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x26b1,
    residue: 0x0
};

/// # [`CRC-16/UMTS`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8005` (reversed: `0xa001`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xfee8`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-umts
pub const CRC_16_UMTS: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8005,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xfee8,
    residue: 0x0
};

/// # [`CRC-16/USB`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x8005` (reversed: `0xa001`)
/// - `init`: `0xffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffff`
/// - `check`: `0xb4c8`
/// - `residue`: `0xb001`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-usb
pub const CRC_16_USB: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x8005,
    init: 0xffff,
    refin: true,
    refout: true,
    xorout: 0xffff,
    check: 0xb4c8,
    residue: 0xb001
};

/// # [`CRC-16/XMODEM`][1]
///
/// - `width`: `16` bits
/// - `poly`: `0x1021` (reversed: `0x8408`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x31c3`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-16-xmodem
pub const CRC_16_XMODEM: Algorithm<u16> = Algorithm {
    width: 16,
    poly: 0x1021,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x31c3,
    residue: 0x0
};

/// # [`CRC-17/CAN-FD`][1]
///
/// - `width`: `17` bits
/// - `poly`: `0x1685b` (reversed: `0x1b42d`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x4f03`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-17-can-fd
pub const CRC_17_CAN_FD: Algorithm<u32> = Algorithm {
    width: 17,
    poly: 0x1685b,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x4f03,
    residue: 0x0
};

/// # [`CRC-21/CAN-FD`][1]
///
/// - `width`: `21` bits
/// - `poly`: `0x102899` (reversed: `0x132281`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xed841`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-21-can-fd
pub const CRC_21_CAN_FD: Algorithm<u32> = Algorithm {
    width: 21,
    poly: 0x102899,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xed841,
    residue: 0x0
};

/// # [`CRC-24/BLE`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x65b` (reversed: `0xda6000`)
/// - `init`: `0x555555`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xc25a56`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-ble
pub const CRC_24_BLE: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x65b,
    init: 0x555555,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xc25a56,
    residue: 0x0
};

/// # [`CRC-24/FLEXRAY-A`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x5d6dcb` (reversed: `0xd3b6ba`)
/// - `init`: `0xfedcba`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x7979bd`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-flexray-a
pub const CRC_24_FLEXRAY_A: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x5d6dcb,
    init: 0xfedcba,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x7979bd,
    residue: 0x0
};

/// # [`CRC-24/FLEXRAY-B`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x5d6dcb` (reversed: `0xd3b6ba`)
/// - `init`: `0xabcdef`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x1f23b8`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-flexray-b
pub const CRC_24_FLEXRAY_B: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x5d6dcb,
    init: 0xabcdef,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x1f23b8,
    residue: 0x0
};

/// # [`CRC-24/INTERLAKEN`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x328b63` (reversed: `0xc6d14c`)
/// - `init`: `0xffffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffffff`
/// - `check`: `0xb4f3e6`
/// - `residue`: `0x144e63`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-interlaken
pub const CRC_24_INTERLAKEN: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x328b63,
    init: 0xffffff,
    refin: false,
    refout: false,
    xorout: 0xffffff,
    check: 0xb4f3e6,
    residue: 0x144e63
};

/// # [`CRC-24/LTE-A`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x864cfb` (reversed: `0xdf3261`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xcde703`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-lte-a
pub const CRC_24_LTE_A: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x864cfb,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xcde703,
    residue: 0x0
};

/// # [`CRC-24/LTE-B`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x800063` (reversed: `0xc60001`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x23ef52`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-lte-b
pub const CRC_24_LTE_B: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x800063,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x23ef52,
    residue: 0x0
};

/// # [`CRC-24/OPENPGP`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x864cfb` (reversed: `0xdf3261`)
/// - `init`: `0xb704ce`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x21cf02`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-openpgp
pub const CRC_24_OPENPGP: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x864cfb,
    init: 0xb704ce,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x21cf02,
    residue: 0x0
};

/// # [`CRC-24/OS-9`][1]
///
/// - `width`: `24` bits
/// - `poly`: `0x800063` (reversed: `0xc60001`)
/// - `init`: `0xffffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffffff`
/// - `check`: `0x200fa5`
/// - `residue`: `0x800fe3`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-24-os-9
pub const CRC_24_OS_9: Algorithm<u32> = Algorithm {
    width: 24,
    poly: 0x800063,
    init: 0xffffff,
    refin: false,
    refout: false,
    xorout: 0xffffff,
    check: 0x200fa5,
    residue: 0x800fe3
};

/// # [`CRC-30/CDMA`][1]
///
/// - `width`: `30` bits
/// - `poly`: `0x2030b9c7` (reversed: `0x38e74301`)
/// - `init`: `0x3fffffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x3fffffff`
/// - `check`: `0x4c34abf`
/// - `residue`: `0x34efa55a`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-30-cdma
pub const CRC_30_CDMA: Algorithm<u32> = Algorithm {
    width: 30,
    poly: 0x2030b9c7,
    init: 0x3fffffff,
    refin: false,
    refout: false,
    xorout: 0x3fffffff,
    check: 0x4c34abf,
    residue: 0x34efa55a
};

/// # [`CRC-31/PHILIPS`][1]
///
/// - `width`: `31` bits
/// - `poly`: `0x4c11db7` (reversed: `0x76dc4190`)
/// - `init`: `0x7fffffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x7fffffff`
/// - `check`: `0xce9e46c`
/// - `residue`: `0x4eaf26f1`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-31-philips
pub const CRC_31_PHILIPS: Algorithm<u32> = Algorithm {
    width: 31,
    poly: 0x4c11db7,
    init: 0x7fffffff,
    refin: false,
    refout: false,
    xorout: 0x7fffffff,
    check: 0xce9e46c,
    residue: 0x4eaf26f1
};

/// # [`CRC-32/AIXM`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x814141ab` (reversed: `0xd5828281`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x3010bf7f`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-aixm
pub const CRC_32_AIXM: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x814141ab,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x3010bf7f,
    residue: 0x0
};

/// # [`CRC-32/AUTOSAR`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0xf4acfb13` (reversed: `0xc8df352f`)
/// - `init`: `0xffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffffffff`
/// - `check`: `0x1697d06a`
/// - `residue`: `0x904cddbf`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-autosar
pub const CRC_32_AUTOSAR: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0xf4acfb13,
    init: 0xffffffff,
    refin: true,
    refout: true,
    xorout: 0xffffffff,
    check: 0x1697d06a,
    residue: 0x904cddbf
};

/// # [`CRC-32/BASE91-D`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0xa833982b` (reversed: `0xd419cc15`)
/// - `init`: `0xffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffffffff`
/// - `check`: `0x87315576`
/// - `residue`: `0x45270551`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-base91-d
pub const CRC_32_BASE91_D: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0xa833982b,
    init: 0xffffffff,
    refin: true,
    refout: true,
    xorout: 0xffffffff,
    check: 0x87315576,
    residue: 0x45270551
};

/// # [`CRC-32/BZIP2`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x4c11db7` (reversed: `0xedb88320`)
/// - `init`: `0xffffffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffffffff`
/// - `check`: `0xfc891918`
/// - `residue`: `0xc704dd7b`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-bzip2
pub const CRC_32_BZIP2: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x4c11db7,
    init: 0xffffffff,
    refin: false,
    refout: false,
    xorout: 0xffffffff,
    check: 0xfc891918,
    residue: 0xc704dd7b
};

/// # [`CRC-32/CD-ROM-EDC`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x8001801b` (reversed: `0xd8018001`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x6ec2edc4`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-cd-rom-edc
pub const CRC_32_CD_ROM_EDC: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x8001801b,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x6ec2edc4,
    residue: 0x0
};

/// # [`CRC-32/CKSUM`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x4c11db7` (reversed: `0xedb88320`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffffffff`
/// - `check`: `0x765e7680`
/// - `residue`: `0xc704dd7b`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-cksum
pub const CRC_32_CKSUM: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x4c11db7,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0xffffffff,
    check: 0x765e7680,
    residue: 0xc704dd7b
};

/// # [`CRC-32/ISCSI`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x1edc6f41` (reversed: `0x82f63b78`)
/// - `init`: `0xffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffffffff`
/// - `check`: `0xe3069283`
/// - `residue`: `0xb798b438`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-iscsi
pub const CRC_32_ISCSI: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x1edc6f41,
    init: 0xffffffff,
    refin: true,
    refout: true,
    xorout: 0xffffffff,
    check: 0xe3069283,
    residue: 0xb798b438
};

/// # [`CRC-32/ISO-HDLC`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x4c11db7` (reversed: `0xedb88320`)
/// - `init`: `0xffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffffffff`
/// - `check`: `0xcbf43926`
/// - `residue`: `0xdebb20e3`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-iso-hdlc
pub const CRC_32_ISO_HDLC: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x4c11db7,
    init: 0xffffffff,
    refin: true,
    refout: true,
    xorout: 0xffffffff,
    check: 0xcbf43926,
    residue: 0xdebb20e3
};

/// # [`CRC-32/JAMCRC`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x4c11db7` (reversed: `0xedb88320`)
/// - `init`: `0xffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x340bc6d9`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-jamcrc
pub const CRC_32_JAMCRC: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x4c11db7,
    init: 0xffffffff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x340bc6d9,
    residue: 0x0
};

/// # [`CRC-32/MEF`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x741b8cd7` (reversed: `0xeb31d82e`)
/// - `init`: `0xffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xd2c22f51`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-mef
pub const CRC_32_MEF: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x741b8cd7,
    init: 0xffffffff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xd2c22f51,
    residue: 0x0
};

/// # [`CRC-32/MPEG-2`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0x4c11db7` (reversed: `0xedb88320`)
/// - `init`: `0xffffffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x376e6e7`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-mpeg-2
pub const CRC_32_MPEG_2: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0x4c11db7,
    init: 0xffffffff,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x376e6e7,
    residue: 0x0
};

/// # [`CRC-32/XFER`][1]
///
/// - `width`: `32` bits
/// - `poly`: `0xaf` (reversed: `0xf5000000`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0xbd0be338`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-32-xfer
pub const CRC_32_XFER: Algorithm<u32> = Algorithm {
    width: 32,
    poly: 0xaf,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0xbd0be338,
    residue: 0x0
};

/// # [`CRC-40/GSM`][1]
///
/// - `width`: `40` bits
/// - `poly`: `0x4820009` (reversed: `0x9000412000`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffffffffff`
/// - `check`: `0xd4164fc646`
/// - `residue`: `0xc4ff8071ff`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-40-gsm
pub const CRC_40_GSM: Algorithm<u64> = Algorithm {
    width: 40,
    poly: 0x4820009,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0xffffffffff,
    check: 0xd4164fc646,
    residue: 0xc4ff8071ff
};

/// # [`CRC-64/ECMA-182`][1]
///
/// - `width`: `64` bits
/// - `poly`: `0x42f0e1eba9ea3693` (reversed: `0xc96c5795d7870f42`)
/// - `init`: `0x0`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0x0`
/// - `check`: `0x6c40df5f0b497347`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-64-ecma-182
pub const CRC_64_ECMA_182: Algorithm<u64> = Algorithm {
    width: 64,
    poly: 0x42f0e1eba9ea3693,
    init: 0x0,
    refin: false,
    refout: false,
    xorout: 0x0,
    check: 0x6c40df5f0b497347,
    residue: 0x0
};

/// # [`CRC-64/GO-ISO`][1]
///
/// - `width`: `64` bits
/// - `poly`: `0x1b` (reversed: `0xd800000000000000`)
/// - `init`: `0xffffffffffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffffffffffffffff`
/// - `check`: `0xb90956c775a41001`
/// - `residue`: `0x5300000000000000`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-64-go-iso
pub const CRC_64_GO_ISO: Algorithm<u64> = Algorithm {
    width: 64,
    poly: 0x1b,
    init: 0xffffffffffffffff,
    refin: true,
    refout: true,
    xorout: 0xffffffffffffffff,
    check: 0xb90956c775a41001,
    residue: 0x5300000000000000
};

/// # [`CRC-64/MS`][1]
///
/// - `width`: `64` bits
/// - `poly`: `0x259c84cba6426349` (reversed: `0x92c64265d32139a4`)
/// - `init`: `0xffffffffffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x75d4b74f024eceea`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-64-ms
pub const CRC_64_MS: Algorithm<u64> = Algorithm {
    width: 64,
    poly: 0x259c84cba6426349,
    init: 0xffffffffffffffff,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x75d4b74f024eceea,
    residue: 0x0
};

/// # [`CRC-64/NVME`][1]
///
/// - `width`: `64` bits
/// - `poly`: `0xad93d23594c93659` (reversed: `0x9a6c9329ac4bc9b5`)
/// - `init`: `0xffffffffffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffffffffffffffff`
/// - `check`: `0xae8b14860a799888`
/// - `residue`: `0xf310303b2b6f6e42`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-64-nvme
pub const CRC_64_NVME: Algorithm<u64> = Algorithm {
    width: 64,
    poly: 0xad93d23594c93659,
    init: 0xffffffffffffffff,
    refin: true,
    refout: true,
    xorout: 0xffffffffffffffff,
    check: 0xae8b14860a799888,
    residue: 0xf310303b2b6f6e42
};

/// # [`CRC-64/REDIS`][1]
///
/// - `width`: `64` bits
/// - `poly`: `0xad93d23594c935a9` (reversed: `0x95ac9329ac4bc9b5`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0xe9c6d914c4b8d9ca`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-64-redis
pub const CRC_64_REDIS: Algorithm<u64> = Algorithm {
    width: 64,
    poly: 0xad93d23594c935a9,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0xe9c6d914c4b8d9ca,
    residue: 0x0
};

/// # [`CRC-64/WE`][1]
///
/// - `width`: `64` bits
/// - `poly`: `0x42f0e1eba9ea3693` (reversed: `0xc96c5795d7870f42`)
/// - `init`: `0xffffffffffffffff`
/// - `refin`: `false`
/// - `refout`: `false`
/// - `xorout`: `0xffffffffffffffff`
/// - `check`: `0x62ec59e3f1a4f00a`
/// - `residue`: `0xfcacbebd5931a992`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-64-we
pub const CRC_64_WE: Algorithm<u64> = Algorithm {
    width: 64,
    poly: 0x42f0e1eba9ea3693,
    init: 0xffffffffffffffff,
    refin: false,
    refout: false,
    xorout: 0xffffffffffffffff,
    check: 0x62ec59e3f1a4f00a,
    residue: 0xfcacbebd5931a992
};

/// # [`CRC-64/XZ`][1]
///
/// - `width`: `64` bits
/// - `poly`: `0x42f0e1eba9ea3693` (reversed: `0xc96c5795d7870f42`)
/// - `init`: `0xffffffffffffffff`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0xffffffffffffffff`
/// - `check`: `0x995dc9bbdf1939fa`
/// - `residue`: `0x49958c9abd7d353f`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-64-xz
pub const CRC_64_XZ: Algorithm<u64> = Algorithm {
    width: 64,
    poly: 0x42f0e1eba9ea3693,
    init: 0xffffffffffffffff,
    refin: true,
    refout: true,
    xorout: 0xffffffffffffffff,
    check: 0x995dc9bbdf1939fa,
    residue: 0x49958c9abd7d353f
};

/// # [`CRC-82/DARC`][1]
///
/// - `width`: `82` bits
/// - `poly`: `0x308c0111011401440411` (reversed: `0x220808a00a2022200c430`)
/// - `init`: `0x0`
/// - `refin`: `true`
/// - `refout`: `true`
/// - `xorout`: `0x0`
/// - `check`: `0x9ea83f625023801fd612`
/// - `residue`: `0x0`
///
/// [1]: https://reveng.sourceforge.io/crc-catalogue/all.htm#crc.cat.crc-82-darc
pub const CRC_82_DARC: Algorithm<u128> = Algorithm {
    width: 82,
    poly: 0x308c0111011401440411,
    init: 0x0,
    refin: true,
    refout: true,
    xorout: 0x0,
    check: 0x9ea83f625023801fd612,
    residue: 0x0
};

