#![allow(non_snake_case)]
use crate::Algorithm;
#[test]
fn test_CRC_3_GSM() {
    pub const CRC_3_GSM: Algorithm<u8> = Algorithm { width: 3, poly: 0x3, init: 0x0, refin: false, refout: false, xorout: 0x7, check: 0x4, residue: 0x2 };
    assert_eq!(CRC_3_GSM, crate::algorithm::CRC_3_GSM);
}
#[test]
fn test_CRC_3_ROHC() {
    pub const CRC_3_ROHC: Algorithm<u8> = Algorithm { width: 3, poly: 0x3, init: 0x7, refin: true, refout: true, xorout: 0x0, check: 0x6, residue: 0x0 };
    assert_eq!(CRC_3_ROHC, crate::algorithm::CRC_3_ROHC);
}
#[test]
fn test_CRC_4_G_704() {
    pub const CRC_4_G_704: Algorithm<u8> = Algorithm { width: 4, poly: 0x3, init: 0x0, refin: true, refout: true, xorout: 0x0, check: 0x7, residue: 0x0 };
    assert_eq!(CRC_4_G_704, crate::algorithm::CRC_4_G_704);
}
#[test]
fn test_CRC_4_INTERLAKEN() {
    pub const CRC_4_INTERLAKEN: Algorithm<u8> = Algorithm { width: 4, poly: 0x3, init: 0xf, refin: false, refout: false, xorout: 0xf, check: 0xb, residue: 0x2 };
    assert_eq!(CRC_4_INTERLAKEN, crate::algorithm::CRC_4_INTERLAKEN);
}
#[test]
fn test_CRC_5_EPC_C1G2() {
    pub const CRC_5_EPC_C1G2: Algorithm<u8> = Algorithm { width: 5, poly: 0x09, init: 0x09, refin: false, refout: false, xorout: 0x00, check: 0x00, residue: 0x00 };
    assert_eq!(CRC_5_EPC_C1G2, crate::algorithm::CRC_5_EPC_C1G2);
}
#[test]
fn test_CRC_5_G_704() {
    pub const CRC_5_G_704: Algorithm<u8> = Algorithm { width: 5, poly: 0x15, init: 0x00, refin: true, refout: true, xorout: 0x00, check: 0x07, residue: 0x00 };
    assert_eq!(CRC_5_G_704, crate::algorithm::CRC_5_G_704);
}
#[test]
fn test_CRC_5_USB() {
    pub const CRC_5_USB: Algorithm<u8> = Algorithm { width: 5, poly: 0x05, init: 0x1f, refin: true, refout: true, xorout: 0x1f, check: 0x19, residue: 0x06 };
    assert_eq!(CRC_5_USB, crate::algorithm::CRC_5_USB);
}
#[test]
fn test_CRC_6_CDMA2000_A() {
    pub const CRC_6_CDMA2000_A: Algorithm<u8> = Algorithm { width: 6, poly: 0x27, init: 0x3f, refin: false, refout: false, xorout: 0x00, check: 0x0d, residue: 0x00 };
    assert_eq!(CRC_6_CDMA2000_A, crate::algorithm::CRC_6_CDMA2000_A);
}
#[test]
fn test_CRC_6_CDMA2000_B() {
    pub const CRC_6_CDMA2000_B: Algorithm<u8> = Algorithm { width: 6, poly: 0x07, init: 0x3f, refin: false, refout: false, xorout: 0x00, check: 0x3b, residue: 0x00 };
    assert_eq!(CRC_6_CDMA2000_B, crate::algorithm::CRC_6_CDMA2000_B);
}
#[test]
fn test_CRC_6_DARC() {
    pub const CRC_6_DARC: Algorithm<u8> = Algorithm { width: 6, poly: 0x19, init: 0x00, refin: true, refout: true, xorout: 0x00, check: 0x26, residue: 0x00 };
    assert_eq!(CRC_6_DARC, crate::algorithm::CRC_6_DARC);
}
#[test]
fn test_CRC_6_G_704() {
    pub const CRC_6_G_704: Algorithm<u8> = Algorithm { width: 6, poly: 0x03, init: 0x00, refin: true, refout: true, xorout: 0x00, check: 0x06, residue: 0x00 };
    assert_eq!(CRC_6_G_704, crate::algorithm::CRC_6_G_704);
}
#[test]
fn test_CRC_6_GSM() {
    pub const CRC_6_GSM: Algorithm<u8> = Algorithm { width: 6, poly: 0x2f, init: 0x00, refin: false, refout: false, xorout: 0x3f, check: 0x13, residue: 0x3a };
    assert_eq!(CRC_6_GSM, crate::algorithm::CRC_6_GSM);
}
#[test]
fn test_CRC_7_MMC() {
    pub const CRC_7_MMC: Algorithm<u8> = Algorithm { width: 7, poly: 0x09, init: 0x00, refin: false, refout: false, xorout: 0x00, check: 0x75, residue: 0x00 };
    assert_eq!(CRC_7_MMC, crate::algorithm::CRC_7_MMC);
}
#[test]
fn test_CRC_7_ROHC() {
    pub const CRC_7_ROHC: Algorithm<u8> = Algorithm { width: 7, poly: 0x4f, init: 0x7f, refin: true, refout: true, xorout: 0x00, check: 0x53, residue: 0x00 };
    assert_eq!(CRC_7_ROHC, crate::algorithm::CRC_7_ROHC);
}
#[test]
fn test_CRC_7_UMTS() {
    pub const CRC_7_UMTS: Algorithm<u8> = Algorithm { width: 7, poly: 0x45, init: 0x00, refin: false, refout: false, xorout: 0x00, check: 0x61, residue: 0x00 };
    assert_eq!(CRC_7_UMTS, crate::algorithm::CRC_7_UMTS);
}
#[test]
fn test_CRC_8_AUTOSAR() {
    pub const CRC_8_AUTOSAR: Algorithm<u8> = Algorithm { width: 8, poly: 0x2f, init: 0xff, refin: false, refout: false, xorout: 0xff, check: 0xdf, residue: 0x42 };
    assert_eq!(CRC_8_AUTOSAR, crate::algorithm::CRC_8_AUTOSAR);
}
#[test]
fn test_CRC_8_BLUETOOTH() {
    pub const CRC_8_BLUETOOTH: Algorithm<u8> = Algorithm { width: 8, poly: 0xa7, init: 0x00, refin: true, refout: true, xorout: 0x00, check: 0x26, residue: 0x00 };
    assert_eq!(CRC_8_BLUETOOTH, crate::algorithm::CRC_8_BLUETOOTH);
}
#[test]
fn test_CRC_8_CDMA2000() {
    pub const CRC_8_CDMA2000: Algorithm<u8> = Algorithm { width: 8, poly: 0x9b, init: 0xff, refin: false, refout: false, xorout: 0x00, check: 0xda, residue: 0x00 };
    assert_eq!(CRC_8_CDMA2000, crate::algorithm::CRC_8_CDMA2000);
}
#[test]
fn test_CRC_8_DARC() {
    pub const CRC_8_DARC: Algorithm<u8> = Algorithm { width: 8, poly: 0x39, init: 0x00, refin: true, refout: true, xorout: 0x00, check: 0x15, residue: 0x00 };
    assert_eq!(CRC_8_DARC, crate::algorithm::CRC_8_DARC);
}
#[test]
fn test_CRC_8_DVB_S2() {
    pub const CRC_8_DVB_S2: Algorithm<u8> = Algorithm { width: 8, poly: 0xd5, init: 0x00, refin: false, refout: false, xorout: 0x00, check: 0xbc, residue: 0x00 };
    assert_eq!(CRC_8_DVB_S2, crate::algorithm::CRC_8_DVB_S2);
}
#[test]
fn test_CRC_8_GSM_A() {
    pub const CRC_8_GSM_A: Algorithm<u8> = Algorithm { width: 8, poly: 0x1d, init: 0x00, refin: false, refout: false, xorout: 0x00, check: 0x37, residue: 0x00 };
    assert_eq!(CRC_8_GSM_A, crate::algorithm::CRC_8_GSM_A);
}
#[test]
fn test_CRC_8_GSM_B() {
    pub const CRC_8_GSM_B: Algorithm<u8> = Algorithm { width: 8, poly: 0x49, init: 0x00, refin: false, refout: false, xorout: 0xff, check: 0x94, residue: 0x53 };
    assert_eq!(CRC_8_GSM_B, crate::algorithm::CRC_8_GSM_B);
}
#[test]
fn test_CRC_8_HITAG() {
    pub const CRC_8_HITAG: Algorithm<u8> = Algorithm { width: 8, poly: 0x1d, init: 0xff, refin: false, refout: false, xorout: 0x00, check: 0xb4, residue: 0x00 };
    assert_eq!(CRC_8_HITAG, crate::algorithm::CRC_8_HITAG);
}
#[test]
fn test_CRC_8_I_432_1() {
    pub const CRC_8_I_432_1: Algorithm<u8> = Algorithm { width: 8, poly: 0x07, init: 0x00, refin: false, refout: false, xorout: 0x55, check: 0xa1, residue: 0xac };
    assert_eq!(CRC_8_I_432_1, crate::algorithm::CRC_8_I_432_1);
}
#[test]
fn test_CRC_8_I_CODE() {
    pub const CRC_8_I_CODE: Algorithm<u8> = Algorithm { width: 8, poly: 0x1d, init: 0xfd, refin: false, refout: false, xorout: 0x00, check: 0x7e, residue: 0x00 };
    assert_eq!(CRC_8_I_CODE, crate::algorithm::CRC_8_I_CODE);
}
#[test]
fn test_CRC_8_LTE() {
    pub const CRC_8_LTE: Algorithm<u8> = Algorithm { width: 8, poly: 0x9b, init: 0x00, refin: false, refout: false, xorout: 0x00, check: 0xea, residue: 0x00 };
    assert_eq!(CRC_8_LTE, crate::algorithm::CRC_8_LTE);
}
#[test]
fn test_CRC_8_MAXIM_DOW() {
    pub const CRC_8_MAXIM_DOW: Algorithm<u8> = Algorithm { width: 8, poly: 0x31, init: 0x00, refin: true, refout: true, xorout: 0x00, check: 0xa1, residue: 0x00 };
    assert_eq!(CRC_8_MAXIM_DOW, crate::algorithm::CRC_8_MAXIM_DOW);
}
#[test]
fn test_CRC_8_MIFARE_MAD() {
    pub const CRC_8_MIFARE_MAD: Algorithm<u8> = Algorithm { width: 8, poly: 0x1d, init: 0xc7, refin: false, refout: false, xorout: 0x00, check: 0x99, residue: 0x00 };
    assert_eq!(CRC_8_MIFARE_MAD, crate::algorithm::CRC_8_MIFARE_MAD);
}
#[test]
fn test_CRC_8_NRSC_5() {
    pub const CRC_8_NRSC_5: Algorithm<u8> = Algorithm { width: 8, poly: 0x31, init: 0xff, refin: false, refout: false, xorout: 0x00, check: 0xf7, residue: 0x00 };
    assert_eq!(CRC_8_NRSC_5, crate::algorithm::CRC_8_NRSC_5);
}
#[test]
fn test_CRC_8_OPENSAFETY() {
    pub const CRC_8_OPENSAFETY: Algorithm<u8> = Algorithm { width: 8, poly: 0x2f, init: 0x00, refin: false, refout: false, xorout: 0x00, check: 0x3e, residue: 0x00 };
    assert_eq!(CRC_8_OPENSAFETY, crate::algorithm::CRC_8_OPENSAFETY);
}
#[test]
fn test_CRC_8_ROHC() {
    pub const CRC_8_ROHC: Algorithm<u8> = Algorithm { width: 8, poly: 0x07, init: 0xff, refin: true, refout: true, xorout: 0x00, check: 0xd0, residue: 0x00 };
    assert_eq!(CRC_8_ROHC, crate::algorithm::CRC_8_ROHC);
}
#[test]
fn test_CRC_8_SAE_J1850() {
    pub const CRC_8_SAE_J1850: Algorithm<u8> = Algorithm { width: 8, poly: 0x1d, init: 0xff, refin: false, refout: false, xorout: 0xff, check: 0x4b, residue: 0xc4 };
    assert_eq!(CRC_8_SAE_J1850, crate::algorithm::CRC_8_SAE_J1850);
}
#[test]
fn test_CRC_8_SMBUS() {
    pub const CRC_8_SMBUS: Algorithm<u8> = Algorithm { width: 8, poly: 0x07, init: 0x00, refin: false, refout: false, xorout: 0x00, check: 0xf4, residue: 0x00 };
    assert_eq!(CRC_8_SMBUS, crate::algorithm::CRC_8_SMBUS);
}
#[test]
fn test_CRC_8_TECH_3250() {
    pub const CRC_8_TECH_3250: Algorithm<u8> = Algorithm { width: 8, poly: 0x1d, init: 0xff, refin: true, refout: true, xorout: 0x00, check: 0x97, residue: 0x00 };
    assert_eq!(CRC_8_TECH_3250, crate::algorithm::CRC_8_TECH_3250);
}
#[test]
fn test_CRC_8_WCDMA() {
    pub const CRC_8_WCDMA: Algorithm<u8> = Algorithm { width: 8, poly: 0x9b, init: 0x00, refin: true, refout: true, xorout: 0x00, check: 0x25, residue: 0x00 };
    assert_eq!(CRC_8_WCDMA, crate::algorithm::CRC_8_WCDMA);
}
#[test]
fn test_CRC_10_ATM() {
    pub const CRC_10_ATM: Algorithm<u16> = Algorithm { width: 10, poly: 0x233, init: 0x000, refin: false, refout: false, xorout: 0x000, check: 0x199, residue: 0x000 };
    assert_eq!(CRC_10_ATM, crate::algorithm::CRC_10_ATM);
}
#[test]
fn test_CRC_10_CDMA2000() {
    pub const CRC_10_CDMA2000: Algorithm<u16> = Algorithm { width: 10, poly: 0x3d9, init: 0x3ff, refin: false, refout: false, xorout: 0x000, check: 0x233, residue: 0x000 };
    assert_eq!(CRC_10_CDMA2000, crate::algorithm::CRC_10_CDMA2000);
}
#[test]
fn test_CRC_10_GSM() {
    pub const CRC_10_GSM: Algorithm<u16> = Algorithm { width: 10, poly: 0x175, init: 0x000, refin: false, refout: false, xorout: 0x3ff, check: 0x12a, residue: 0x0c6 };
    assert_eq!(CRC_10_GSM, crate::algorithm::CRC_10_GSM);
}
#[test]
fn test_CRC_11_FLEXRAY() {
    pub const CRC_11_FLEXRAY: Algorithm<u16> = Algorithm { width: 11, poly: 0x385, init: 0x01a, refin: false, refout: false, xorout: 0x000, check: 0x5a3, residue: 0x000 };
    assert_eq!(CRC_11_FLEXRAY, crate::algorithm::CRC_11_FLEXRAY);
}
#[test]
fn test_CRC_11_UMTS() {
    pub const CRC_11_UMTS: Algorithm<u16> = Algorithm { width: 11, poly: 0x307, init: 0x000, refin: false, refout: false, xorout: 0x000, check: 0x061, residue: 0x000 };
    assert_eq!(CRC_11_UMTS, crate::algorithm::CRC_11_UMTS);
}
#[test]
fn test_CRC_12_CDMA2000() {
    pub const CRC_12_CDMA2000: Algorithm<u16> = Algorithm { width: 12, poly: 0xf13, init: 0xfff, refin: false, refout: false, xorout: 0x000, check: 0xd4d, residue: 0x000 };
    assert_eq!(CRC_12_CDMA2000, crate::algorithm::CRC_12_CDMA2000);
}
#[test]
fn test_CRC_12_DECT() {
    pub const CRC_12_DECT: Algorithm<u16> = Algorithm { width: 12, poly: 0x80f, init: 0x000, refin: false, refout: false, xorout: 0x000, check: 0xf5b, residue: 0x000 };
    assert_eq!(CRC_12_DECT, crate::algorithm::CRC_12_DECT);
}
#[test]
fn test_CRC_12_GSM() {
    pub const CRC_12_GSM: Algorithm<u16> = Algorithm { width: 12, poly: 0xd31, init: 0x000, refin: false, refout: false, xorout: 0xfff, check: 0xb34, residue: 0x178 };
    assert_eq!(CRC_12_GSM, crate::algorithm::CRC_12_GSM);
}
#[test]
fn test_CRC_12_UMTS() {
    pub const CRC_12_UMTS: Algorithm<u16> = Algorithm { width: 12, poly: 0x80f, init: 0x000, refin: false, refout: true, xorout: 0x000, check: 0xdaf, residue: 0x000 };
    assert_eq!(CRC_12_UMTS, crate::algorithm::CRC_12_UMTS);
}
#[test]
fn test_CRC_13_BBC() {
    pub const CRC_13_BBC: Algorithm<u16> = Algorithm { width: 13, poly: 0x1cf5, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0x04fa, residue: 0x0000 };
    assert_eq!(CRC_13_BBC, crate::algorithm::CRC_13_BBC);
}
#[test]
fn test_CRC_14_DARC() {
    pub const CRC_14_DARC: Algorithm<u16> = Algorithm { width: 14, poly: 0x0805, init: 0x0000, refin: true, refout: true, xorout: 0x0000, check: 0x082d, residue: 0x0000 };
    assert_eq!(CRC_14_DARC, crate::algorithm::CRC_14_DARC);
}
#[test]
fn test_CRC_14_GSM() {
    pub const CRC_14_GSM: Algorithm<u16> = Algorithm { width: 14, poly: 0x202d, init: 0x0000, refin: false, refout: false, xorout: 0x3fff, check: 0x30ae, residue: 0x031e };
    assert_eq!(CRC_14_GSM, crate::algorithm::CRC_14_GSM);
}
#[test]
fn test_CRC_15_CAN() {
    pub const CRC_15_CAN: Algorithm<u16> = Algorithm { width: 15, poly: 0x4599, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0x059e, residue: 0x0000 };
    assert_eq!(CRC_15_CAN, crate::algorithm::CRC_15_CAN);
}
#[test]
fn test_CRC_15_MPT1327() {
    pub const CRC_15_MPT1327: Algorithm<u16> = Algorithm { width: 15, poly: 0x6815, init: 0x0000, refin: false, refout: false, xorout: 0x0001, check: 0x2566, residue: 0x6815 };
    assert_eq!(CRC_15_MPT1327, crate::algorithm::CRC_15_MPT1327);
}
#[test]
fn test_CRC_16_ARC() {
    pub const CRC_16_ARC: Algorithm<u16> = Algorithm { width: 16, poly: 0x8005, init: 0x0000, refin: true, refout: true, xorout: 0x0000, check: 0xbb3d, residue: 0x0000 };
    assert_eq!(CRC_16_ARC, crate::algorithm::CRC_16_ARC);
}
#[test]
fn test_CRC_16_CDMA2000() {
    pub const CRC_16_CDMA2000: Algorithm<u16> = Algorithm { width: 16, poly: 0xc867, init: 0xffff, refin: false, refout: false, xorout: 0x0000, check: 0x4c06, residue: 0x0000 };
    assert_eq!(CRC_16_CDMA2000, crate::algorithm::CRC_16_CDMA2000);
}
#[test]
fn test_CRC_16_CMS() {
    pub const CRC_16_CMS: Algorithm<u16> = Algorithm { width: 16, poly: 0x8005, init: 0xffff, refin: false, refout: false, xorout: 0x0000, check: 0xaee7, residue: 0x0000 };
    assert_eq!(CRC_16_CMS, crate::algorithm::CRC_16_CMS);
}
#[test]
fn test_CRC_16_DDS_110() {
    pub const CRC_16_DDS_110: Algorithm<u16> = Algorithm { width: 16, poly: 0x8005, init: 0x800d, refin: false, refout: false, xorout: 0x0000, check: 0x9ecf, residue: 0x0000 };
    assert_eq!(CRC_16_DDS_110, crate::algorithm::CRC_16_DDS_110);
}
#[test]
fn test_CRC_16_DECT_R() {
    pub const CRC_16_DECT_R: Algorithm<u16> = Algorithm { width: 16, poly: 0x0589, init: 0x0000, refin: false, refout: false, xorout: 0x0001, check: 0x007e, residue: 0x0589 };
    assert_eq!(CRC_16_DECT_R, crate::algorithm::CRC_16_DECT_R);
}
#[test]
fn test_CRC_16_DECT_X() {
    pub const CRC_16_DECT_X: Algorithm<u16> = Algorithm { width: 16, poly: 0x0589, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0x007f, residue: 0x0000 };
    assert_eq!(CRC_16_DECT_X, crate::algorithm::CRC_16_DECT_X);
}
#[test]
fn test_CRC_16_DNP() {
    pub const CRC_16_DNP: Algorithm<u16> = Algorithm { width: 16, poly: 0x3d65, init: 0x0000, refin: true, refout: true, xorout: 0xffff, check: 0xea82, residue: 0x66c5 };
    assert_eq!(CRC_16_DNP, crate::algorithm::CRC_16_DNP);
}
#[test]
fn test_CRC_16_EN_13757() {
    pub const CRC_16_EN_13757: Algorithm<u16> = Algorithm { width: 16, poly: 0x3d65, init: 0x0000, refin: false, refout: false, xorout: 0xffff, check: 0xc2b7, residue: 0xa366 };
    assert_eq!(CRC_16_EN_13757, crate::algorithm::CRC_16_EN_13757);
}
#[test]
fn test_CRC_16_GENIBUS() {
    pub const CRC_16_GENIBUS: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0xffff, refin: false, refout: false, xorout: 0xffff, check: 0xd64e, residue: 0x1d0f };
    assert_eq!(CRC_16_GENIBUS, crate::algorithm::CRC_16_GENIBUS);
}
#[test]
fn test_CRC_16_GSM() {
    pub const CRC_16_GSM: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0x0000, refin: false, refout: false, xorout: 0xffff, check: 0xce3c, residue: 0x1d0f };
    assert_eq!(CRC_16_GSM, crate::algorithm::CRC_16_GSM);
}
#[test]
fn test_CRC_16_IBM_3740() {
    pub const CRC_16_IBM_3740: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0xffff, refin: false, refout: false, xorout: 0x0000, check: 0x29b1, residue: 0x0000 };
    assert_eq!(CRC_16_IBM_3740, crate::algorithm::CRC_16_IBM_3740);
}
#[test]
fn test_CRC_16_IBM_SDLC() {
    pub const CRC_16_IBM_SDLC: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0xffff, refin: true, refout: true, xorout: 0xffff, check: 0x906e, residue: 0xf0b8 };
    assert_eq!(CRC_16_IBM_SDLC, crate::algorithm::CRC_16_IBM_SDLC);
}
#[test]
fn test_CRC_16_ISO_IEC_14443_3_A() {
    pub const CRC_16_ISO_IEC_14443_3_A: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0xc6c6, refin: true, refout: true, xorout: 0x0000, check: 0xbf05, residue: 0x0000 };
    assert_eq!(CRC_16_ISO_IEC_14443_3_A, crate::algorithm::CRC_16_ISO_IEC_14443_3_A);
}
#[test]
fn test_CRC_16_KERMIT() {
    pub const CRC_16_KERMIT: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0x0000, refin: true, refout: true, xorout: 0x0000, check: 0x2189, residue: 0x0000 };
    assert_eq!(CRC_16_KERMIT, crate::algorithm::CRC_16_KERMIT);
}
#[test]
fn test_CRC_16_LJ1200() {
    pub const CRC_16_LJ1200: Algorithm<u16> = Algorithm { width: 16, poly: 0x6f63, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0xbdf4, residue: 0x0000 };
    assert_eq!(CRC_16_LJ1200, crate::algorithm::CRC_16_LJ1200);
}
#[test]
fn test_CRC_16_M17() {
    pub const CRC_16_M17: Algorithm<u16> = Algorithm { width: 16, poly: 0x5935, init: 0xffff, refin: false, refout: false, xorout: 0x0000, check: 0x772b, residue: 0x0000 };
    assert_eq!(CRC_16_M17, crate::algorithm::CRC_16_M17);
}
#[test]
fn test_CRC_16_MAXIM_DOW() {
    pub const CRC_16_MAXIM_DOW: Algorithm<u16> = Algorithm { width: 16, poly: 0x8005, init: 0x0000, refin: true, refout: true, xorout: 0xffff, check: 0x44c2, residue: 0xb001 };
    assert_eq!(CRC_16_MAXIM_DOW, crate::algorithm::CRC_16_MAXIM_DOW);
}
#[test]
fn test_CRC_16_MCRF4XX() {
    pub const CRC_16_MCRF4XX: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0xffff, refin: true, refout: true, xorout: 0x0000, check: 0x6f91, residue: 0x0000 };
    assert_eq!(CRC_16_MCRF4XX, crate::algorithm::CRC_16_MCRF4XX);
}
#[test]
fn test_CRC_16_MODBUS() {
    pub const CRC_16_MODBUS: Algorithm<u16> = Algorithm { width: 16, poly: 0x8005, init: 0xffff, refin: true, refout: true, xorout: 0x0000, check: 0x4b37, residue: 0x0000 };
    assert_eq!(CRC_16_MODBUS, crate::algorithm::CRC_16_MODBUS);
}
#[test]
fn test_CRC_16_NRSC_5() {
    pub const CRC_16_NRSC_5: Algorithm<u16> = Algorithm { width: 16, poly: 0x080b, init: 0xffff, refin: true, refout: true, xorout: 0x0000, check: 0xa066, residue: 0x0000 };
    assert_eq!(CRC_16_NRSC_5, crate::algorithm::CRC_16_NRSC_5);
}
#[test]
fn test_CRC_16_OPENSAFETY_A() {
    pub const CRC_16_OPENSAFETY_A: Algorithm<u16> = Algorithm { width: 16, poly: 0x5935, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0x5d38, residue: 0x0000 };
    assert_eq!(CRC_16_OPENSAFETY_A, crate::algorithm::CRC_16_OPENSAFETY_A);
}
#[test]
fn test_CRC_16_OPENSAFETY_B() {
    pub const CRC_16_OPENSAFETY_B: Algorithm<u16> = Algorithm { width: 16, poly: 0x755b, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0x20fe, residue: 0x0000 };
    assert_eq!(CRC_16_OPENSAFETY_B, crate::algorithm::CRC_16_OPENSAFETY_B);
}
#[test]
fn test_CRC_16_PROFIBUS() {
    pub const CRC_16_PROFIBUS: Algorithm<u16> = Algorithm { width: 16, poly: 0x1dcf, init: 0xffff, refin: false, refout: false, xorout: 0xffff, check: 0xa819, residue: 0xe394 };
    assert_eq!(CRC_16_PROFIBUS, crate::algorithm::CRC_16_PROFIBUS);
}
#[test]
fn test_CRC_16_RIELLO() {
    pub const CRC_16_RIELLO: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0xb2aa, refin: true, refout: true, xorout: 0x0000, check: 0x63d0, residue: 0x0000 };
    assert_eq!(CRC_16_RIELLO, crate::algorithm::CRC_16_RIELLO);
}
#[test]
fn test_CRC_16_SPI_FUJITSU() {
    pub const CRC_16_SPI_FUJITSU: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0x1d0f, refin: false, refout: false, xorout: 0x0000, check: 0xe5cc, residue: 0x0000 };
    assert_eq!(CRC_16_SPI_FUJITSU, crate::algorithm::CRC_16_SPI_FUJITSU);
}
#[test]
fn test_CRC_16_T10_DIF() {
    pub const CRC_16_T10_DIF: Algorithm<u16> = Algorithm { width: 16, poly: 0x8bb7, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0xd0db, residue: 0x0000 };
    assert_eq!(CRC_16_T10_DIF, crate::algorithm::CRC_16_T10_DIF);
}
#[test]
fn test_CRC_16_TELEDISK() {
    pub const CRC_16_TELEDISK: Algorithm<u16> = Algorithm { width: 16, poly: 0xa097, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0x0fb3, residue: 0x0000 };
    assert_eq!(CRC_16_TELEDISK, crate::algorithm::CRC_16_TELEDISK);
}
#[test]
fn test_CRC_16_TMS37157() {
    pub const CRC_16_TMS37157: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0x89ec, refin: true, refout: true, xorout: 0x0000, check: 0x26b1, residue: 0x0000 };
    assert_eq!(CRC_16_TMS37157, crate::algorithm::CRC_16_TMS37157);
}
#[test]
fn test_CRC_16_UMTS() {
    pub const CRC_16_UMTS: Algorithm<u16> = Algorithm { width: 16, poly: 0x8005, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0xfee8, residue: 0x0000 };
    assert_eq!(CRC_16_UMTS, crate::algorithm::CRC_16_UMTS);
}
#[test]
fn test_CRC_16_USB() {
    pub const CRC_16_USB: Algorithm<u16> = Algorithm { width: 16, poly: 0x8005, init: 0xffff, refin: true, refout: true, xorout: 0xffff, check: 0xb4c8, residue: 0xb001 };
    assert_eq!(CRC_16_USB, crate::algorithm::CRC_16_USB);
}
#[test]
fn test_CRC_16_XMODEM() {
    pub const CRC_16_XMODEM: Algorithm<u16> = Algorithm { width: 16, poly: 0x1021, init: 0x0000, refin: false, refout: false, xorout: 0x0000, check: 0x31c3, residue: 0x0000 };
    assert_eq!(CRC_16_XMODEM, crate::algorithm::CRC_16_XMODEM);
}
#[test]
fn test_CRC_17_CAN_FD() {
    pub const CRC_17_CAN_FD: Algorithm<u32> = Algorithm { width: 17, poly: 0x1685b, init: 0x00000, refin: false, refout: false, xorout: 0x00000, check: 0x04f03, residue: 0x00000 };
    assert_eq!(CRC_17_CAN_FD, crate::algorithm::CRC_17_CAN_FD);
}
#[test]
fn test_CRC_21_CAN_FD() {
    pub const CRC_21_CAN_FD: Algorithm<u32> = Algorithm { width: 21, poly: 0x102899, init: 0x000000, refin: false, refout: false, xorout: 0x000000, check: 0x0ed841, residue: 0x000000 };
    assert_eq!(CRC_21_CAN_FD, crate::algorithm::CRC_21_CAN_FD);
}
#[test]
fn test_CRC_24_BLE() {
    pub const CRC_24_BLE: Algorithm<u32> = Algorithm { width: 24, poly: 0x00065b, init: 0x555555, refin: true, refout: true, xorout: 0x000000, check: 0xc25a56, residue: 0x000000 };
    assert_eq!(CRC_24_BLE, crate::algorithm::CRC_24_BLE);
}
#[test]
fn test_CRC_24_FLEXRAY_A() {
    pub const CRC_24_FLEXRAY_A: Algorithm<u32> = Algorithm { width: 24, poly: 0x5d6dcb, init: 0xfedcba, refin: false, refout: false, xorout: 0x000000, check: 0x7979bd, residue: 0x000000 };
    assert_eq!(CRC_24_FLEXRAY_A, crate::algorithm::CRC_24_FLEXRAY_A);
}
#[test]
fn test_CRC_24_FLEXRAY_B() {
    pub const CRC_24_FLEXRAY_B: Algorithm<u32> = Algorithm { width: 24, poly: 0x5d6dcb, init: 0xabcdef, refin: false, refout: false, xorout: 0x000000, check: 0x1f23b8, residue: 0x000000 };
    assert_eq!(CRC_24_FLEXRAY_B, crate::algorithm::CRC_24_FLEXRAY_B);
}
#[test]
fn test_CRC_24_INTERLAKEN() {
    pub const CRC_24_INTERLAKEN: Algorithm<u32> = Algorithm { width: 24, poly: 0x328b63, init: 0xffffff, refin: false, refout: false, xorout: 0xffffff, check: 0xb4f3e6, residue: 0x144e63 };
    assert_eq!(CRC_24_INTERLAKEN, crate::algorithm::CRC_24_INTERLAKEN);
}
#[test]
fn test_CRC_24_LTE_A() {
    pub const CRC_24_LTE_A: Algorithm<u32> = Algorithm { width: 24, poly: 0x864cfb, init: 0x000000, refin: false, refout: false, xorout: 0x000000, check: 0xcde703, residue: 0x000000 };
    assert_eq!(CRC_24_LTE_A, crate::algorithm::CRC_24_LTE_A);
}
#[test]
fn test_CRC_24_LTE_B() {
    pub const CRC_24_LTE_B: Algorithm<u32> = Algorithm { width: 24, poly: 0x800063, init: 0x000000, refin: false, refout: false, xorout: 0x000000, check: 0x23ef52, residue: 0x000000 };
    assert_eq!(CRC_24_LTE_B, crate::algorithm::CRC_24_LTE_B);
}
#[test]
fn test_CRC_24_OPENPGP() {
    pub const CRC_24_OPENPGP: Algorithm<u32> = Algorithm { width: 24, poly: 0x864cfb, init: 0xb704ce, refin: false, refout: false, xorout: 0x000000, check: 0x21cf02, residue: 0x000000 };
    assert_eq!(CRC_24_OPENPGP, crate::algorithm::CRC_24_OPENPGP);
}
#[test]
fn test_CRC_24_OS_9() {
    pub const CRC_24_OS_9: Algorithm<u32> = Algorithm { width: 24, poly: 0x800063, init: 0xffffff, refin: false, refout: false, xorout: 0xffffff, check: 0x200fa5, residue: 0x800fe3 };
    assert_eq!(CRC_24_OS_9, crate::algorithm::CRC_24_OS_9);
}
#[test]
fn test_CRC_30_CDMA() {
    pub const CRC_30_CDMA: Algorithm<u32> = Algorithm { width: 30, poly: 0x2030b9c7, init: 0x3fffffff, refin: false, refout: false, xorout: 0x3fffffff, check: 0x04c34abf, residue: 0x34efa55a };
    assert_eq!(CRC_30_CDMA, crate::algorithm::CRC_30_CDMA);
}
#[test]
fn test_CRC_31_PHILIPS() {
    pub const CRC_31_PHILIPS: Algorithm<u32> = Algorithm { width: 31, poly: 0x04c11db7, init: 0x7fffffff, refin: false, refout: false, xorout: 0x7fffffff, check: 0x0ce9e46c, residue: 0x4eaf26f1 };
    assert_eq!(CRC_31_PHILIPS, crate::algorithm::CRC_31_PHILIPS);
}
#[test]
fn test_CRC_32_AIXM() {
    pub const CRC_32_AIXM: Algorithm<u32> = Algorithm { width: 32, poly: 0x814141ab, init: 0x00000000, refin: false, refout: false, xorout: 0x00000000, check: 0x3010bf7f, residue: 0x00000000 };
    assert_eq!(CRC_32_AIXM, crate::algorithm::CRC_32_AIXM);
}
#[test]
fn test_CRC_32_AUTOSAR() {
    pub const CRC_32_AUTOSAR: Algorithm<u32> = Algorithm { width: 32, poly: 0xf4acfb13, init: 0xffffffff, refin: true, refout: true, xorout: 0xffffffff, check: 0x1697d06a, residue: 0x904cddbf };
    assert_eq!(CRC_32_AUTOSAR, crate::algorithm::CRC_32_AUTOSAR);
}
#[test]
fn test_CRC_32_BASE91_D() {
    pub const CRC_32_BASE91_D: Algorithm<u32> = Algorithm { width: 32, poly: 0xa833982b, init: 0xffffffff, refin: true, refout: true, xorout: 0xffffffff, check: 0x87315576, residue: 0x45270551 };
    assert_eq!(CRC_32_BASE91_D, crate::algorithm::CRC_32_BASE91_D);
}
#[test]
fn test_CRC_32_BZIP2() {
    pub const CRC_32_BZIP2: Algorithm<u32> = Algorithm { width: 32, poly: 0x04c11db7, init: 0xffffffff, refin: false, refout: false, xorout: 0xffffffff, check: 0xfc891918, residue: 0xc704dd7b };
    assert_eq!(CRC_32_BZIP2, crate::algorithm::CRC_32_BZIP2);
}
#[test]
fn test_CRC_32_CD_ROM_EDC() {
    pub const CRC_32_CD_ROM_EDC: Algorithm<u32> = Algorithm { width: 32, poly: 0x8001801b, init: 0x00000000, refin: true, refout: true, xorout: 0x00000000, check: 0x6ec2edc4, residue: 0x00000000 };
    assert_eq!(CRC_32_CD_ROM_EDC, crate::algorithm::CRC_32_CD_ROM_EDC);
}
#[test]
fn test_CRC_32_CKSUM() {
    pub const CRC_32_CKSUM: Algorithm<u32> = Algorithm { width: 32, poly: 0x04c11db7, init: 0x00000000, refin: false, refout: false, xorout: 0xffffffff, check: 0x765e7680, residue: 0xc704dd7b };
    assert_eq!(CRC_32_CKSUM, crate::algorithm::CRC_32_CKSUM);
}
#[test]
fn test_CRC_32_ISCSI() {
    pub const CRC_32_ISCSI: Algorithm<u32> = Algorithm { width: 32, poly: 0x1edc6f41, init: 0xffffffff, refin: true, refout: true, xorout: 0xffffffff, check: 0xe3069283, residue: 0xb798b438 };
    assert_eq!(CRC_32_ISCSI, crate::algorithm::CRC_32_ISCSI);
}
#[test]
fn test_CRC_32_ISO_HDLC() {
    pub const CRC_32_ISO_HDLC: Algorithm<u32> = Algorithm { width: 32, poly: 0x04c11db7, init: 0xffffffff, refin: true, refout: true, xorout: 0xffffffff, check: 0xcbf43926, residue: 0xdebb20e3 };
    assert_eq!(CRC_32_ISO_HDLC, crate::algorithm::CRC_32_ISO_HDLC);
}
#[test]
fn test_CRC_32_JAMCRC() {
    pub const CRC_32_JAMCRC: Algorithm<u32> = Algorithm { width: 32, poly: 0x04c11db7, init: 0xffffffff, refin: true, refout: true, xorout: 0x00000000, check: 0x340bc6d9, residue: 0x00000000 };
    assert_eq!(CRC_32_JAMCRC, crate::algorithm::CRC_32_JAMCRC);
}
#[test]
fn test_CRC_32_MEF() {
    pub const CRC_32_MEF: Algorithm<u32> = Algorithm { width: 32, poly: 0x741b8cd7, init: 0xffffffff, refin: true, refout: true, xorout: 0x00000000, check: 0xd2c22f51, residue: 0x00000000 };
    assert_eq!(CRC_32_MEF, crate::algorithm::CRC_32_MEF);
}
#[test]
fn test_CRC_32_MPEG_2() {
    pub const CRC_32_MPEG_2: Algorithm<u32> = Algorithm { width: 32, poly: 0x04c11db7, init: 0xffffffff, refin: false, refout: false, xorout: 0x00000000, check: 0x0376e6e7, residue: 0x00000000 };
    assert_eq!(CRC_32_MPEG_2, crate::algorithm::CRC_32_MPEG_2);
}
#[test]
fn test_CRC_32_XFER() {
    pub const CRC_32_XFER: Algorithm<u32> = Algorithm { width: 32, poly: 0x000000af, init: 0x00000000, refin: false, refout: false, xorout: 0x00000000, check: 0xbd0be338, residue: 0x00000000 };
    assert_eq!(CRC_32_XFER, crate::algorithm::CRC_32_XFER);
}
#[test]
fn test_CRC_40_GSM() {
    pub const CRC_40_GSM: Algorithm<u64> = Algorithm { width: 40, poly: 0x0004820009, init: 0x0000000000, refin: false, refout: false, xorout: 0xffffffffff, check: 0xd4164fc646, residue: 0xc4ff8071ff };
    assert_eq!(CRC_40_GSM, crate::algorithm::CRC_40_GSM);
}
#[test]
fn test_CRC_64_ECMA_182() {
    pub const CRC_64_ECMA_182: Algorithm<u64> = Algorithm { width: 64, poly: 0x42f0e1eba9ea3693, init: 0x0000000000000000, refin: false, refout: false, xorout: 0x0000000000000000, check: 0x6c40df5f0b497347, residue: 0x0000000000000000 };
    assert_eq!(CRC_64_ECMA_182, crate::algorithm::CRC_64_ECMA_182);
}
#[test]
fn test_CRC_64_GO_ISO() {
    pub const CRC_64_GO_ISO: Algorithm<u64> = Algorithm { width: 64, poly: 0x000000000000001b, init: 0xffffffffffffffff, refin: true, refout: true, xorout: 0xffffffffffffffff, check: 0xb90956c775a41001, residue: 0x5300000000000000 };
    assert_eq!(CRC_64_GO_ISO, crate::algorithm::CRC_64_GO_ISO);
}
#[test]
fn test_CRC_64_MS() {
    pub const CRC_64_MS: Algorithm<u64> = Algorithm { width: 64, poly: 0x259c84cba6426349, init: 0xffffffffffffffff, refin: true, refout: true, xorout: 0x0000000000000000, check: 0x75d4b74f024eceea, residue: 0x0000000000000000 };
    assert_eq!(CRC_64_MS, crate::algorithm::CRC_64_MS);
}
#[test]
fn test_CRC_64_REDIS() {
    pub const CRC_64_REDIS: Algorithm<u64> = Algorithm { width: 64, poly: 0xad93d23594c935a9, init: 0x0000000000000000, refin: true, refout: true, xorout: 0x0000000000000000, check: 0xe9c6d914c4b8d9ca, residue: 0x0000000000000000 };
    assert_eq!(CRC_64_REDIS, crate::algorithm::CRC_64_REDIS);
}
#[test]
fn test_CRC_64_WE() {
    pub const CRC_64_WE: Algorithm<u64> = Algorithm { width: 64, poly: 0x42f0e1eba9ea3693, init: 0xffffffffffffffff, refin: false, refout: false, xorout: 0xffffffffffffffff, check: 0x62ec59e3f1a4f00a, residue: 0xfcacbebd5931a992 };
    assert_eq!(CRC_64_WE, crate::algorithm::CRC_64_WE);
}
#[test]
fn test_CRC_64_XZ() {
    pub const CRC_64_XZ: Algorithm<u64> = Algorithm { width: 64, poly: 0x42f0e1eba9ea3693, init: 0xffffffffffffffff, refin: true, refout: true, xorout: 0xffffffffffffffff, check: 0x995dc9bbdf1939fa, residue: 0x49958c9abd7d353f };
    assert_eq!(CRC_64_XZ, crate::algorithm::CRC_64_XZ);
}
#[test]
fn test_CRC_82_DARC() {
    pub const CRC_82_DARC: Algorithm<u128> = Algorithm { width: 82, poly: 0x0308c0111011401440411, init: 0x000000000000000000000, refin: true, refout: true, xorout: 0x000000000000000000000, check: 0x09ea83f625023801fd612, residue: 0x000000000000000000000 };
    assert_eq!(CRC_82_DARC, crate::algorithm::CRC_82_DARC);
}
