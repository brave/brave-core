use core::array;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum AesType {
    AES128,
    AES192,
    AES256,
}

pub(crate) struct TestVector {
    pub(crate) aes: AesType,
    pub(crate) key: Vec<u8>,
    pub(crate) radix: u32,
    pub(crate) tweak: Vec<u8>,
    pub(crate) pt: Vec<u16>,
    pub(crate) ct: Vec<u16>,
    pub(crate) binary: Option<BinaryTestVector>,
}

pub(crate) struct BinaryTestVector {
    pub(crate) pt: Vec<u8>,
    pub(crate) ct: Vec<u8>,
}

pub(crate) fn get() -> impl Iterator<Item = TestVector> {
    #[allow(deprecated)]
    array::IntoIter::new([
        // From https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/FF1samples.pdf
        TestVector {
            // Sample #1
            aes: AesType::AES128,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C,
            ],
            radix: 10,
            tweak: vec![],
            pt: vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            ct: vec![2, 4, 3, 3, 4, 7, 7, 4, 8, 4],
            binary: None,
        },
        TestVector {
            // Sample #2
            aes: AesType::AES128,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C,
            ],
            radix: 10,
            tweak: vec![0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30],
            pt: vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            ct: vec![6, 1, 2, 4, 2, 0, 0, 7, 7, 3],
            binary: None,
        },
        TestVector {
            // Sample #3
            aes: AesType::AES128,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C,
            ],
            radix: 36,
            tweak: vec![
                0x37, 0x37, 0x37, 0x37, 0x70, 0x71, 0x72, 0x73, 0x37, 0x37, 0x37,
            ],
            pt: vec![
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
            ],
            ct: vec![
                10, 9, 29, 31, 4, 0, 22, 21, 21, 9, 20, 13, 30, 5, 0, 9, 14, 30, 22,
            ],
            binary: None,
        },
        TestVector {
            // Sample #4
            aes: AesType::AES192,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F,
            ],
            radix: 10,
            tweak: vec![],
            pt: vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            ct: vec![2, 8, 3, 0, 6, 6, 8, 1, 3, 2],
            binary: None,
        },
        TestVector {
            // Sample #5
            aes: AesType::AES192,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F,
            ],
            radix: 10,
            tweak: vec![0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30],
            pt: vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            ct: vec![2, 4, 9, 6, 6, 5, 5, 5, 4, 9],
            binary: None,
        },
        TestVector {
            // Sample #6
            aes: AesType::AES192,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F,
            ],
            radix: 36,
            tweak: vec![
                0x37, 0x37, 0x37, 0x37, 0x70, 0x71, 0x72, 0x73, 0x37, 0x37, 0x37,
            ],
            pt: vec![
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
            ],
            ct: vec![
                33, 11, 19, 3, 20, 31, 3, 5, 19, 27, 10, 32, 33, 31, 3, 2, 34, 28, 27,
            ],
            binary: None,
        },
        TestVector {
            // Sample #7
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 10,
            tweak: vec![],
            pt: vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            ct: vec![6, 6, 5, 7, 6, 6, 7, 0, 0, 9],
            binary: None,
        },
        TestVector {
            // Sample #8
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 10,
            tweak: vec![0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30],
            pt: vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            ct: vec![1, 0, 0, 1, 6, 2, 3, 4, 6, 3],
            binary: None,
        },
        TestVector {
            // Sample #9
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 36,
            tweak: vec![
                0x37, 0x37, 0x37, 0x37, 0x70, 0x71, 0x72, 0x73, 0x37, 0x37, 0x37,
            ],
            pt: vec![
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
            ],
            ct: vec![
                33, 28, 8, 10, 0, 10, 35, 17, 2, 10, 31, 34, 10, 21, 34, 35, 30, 32, 13,
            ],
            binary: None,
        },
        // From https://github.com/capitalone/fpe/blob/master/ff1/ff1_test.go
        TestVector {
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 36,
            tweak: vec![],
            pt: vec![
                33, 28, 8, 10, 0, 10, 35, 17, 2, 10, 31, 34, 10, 21, 34, 35, 30, 32, 13, 33, 28, 8,
                10, 0, 10, 35, 17, 2, 10, 31, 34, 10, 21, 34, 35, 30, 32, 13, 33, 28, 8, 10, 0, 10,
                35, 17, 2, 10, 31, 34, 10, 21, 34, 35, 30, 32, 13, 33, 28, 8, 10, 0, 10, 35, 17, 2,
                10, 31, 34, 10, 21, 34, 35, 30, 32, 13, 33, 28, 8, 10, 0, 10, 35, 17, 2, 10, 31,
                34, 10, 21, 34, 35, 30, 32, 13, 33, 28, 8, 10, 0, 10, 35, 17, 2, 10, 31, 34, 10,
                21, 34, 35, 30, 32, 13, 33, 28, 8, 10, 0, 10, 35, 17, 2, 10, 31, 34, 10, 21,
            ],
            // lwulibfp1ju3ksztumqomwenpv7duy9q7pg7zf3eg3rjlfy46gmgkqjfwvromfjjktmbey8meqk9zkcmgvkv4s9ll5ctozme1hf15w7xo6zsylqcr0nbx9jbf10umzok
            ct: vec![
                21, 32, 30, 21, 18, 11, 15, 25, 1, 19, 30, 3, 20, 28, 35, 29, 30, 22, 26, 24, 22,
                32, 14, 23, 25, 31, 7, 13, 30, 34, 9, 26, 7, 25, 16, 7, 35, 15, 3, 14, 16, 3, 27,
                19, 21, 15, 34, 4, 6, 16, 22, 16, 20, 26, 19, 15, 32, 31, 27, 24, 22, 15, 19, 19,
                20, 29, 22, 11, 14, 34, 8, 22, 14, 26, 20, 9, 35, 20, 12, 22, 16, 31, 20, 31, 4,
                28, 9, 21, 21, 5, 12, 29, 24, 35, 22, 14, 1, 17, 15, 1, 5, 32, 7, 33, 24, 6, 35,
                28, 34, 21, 26, 12, 27, 0, 23, 11, 33, 9, 19, 11, 15, 1, 0, 30, 22, 35, 24, 20,
            ],
            binary: None,
        },
        // Zcash test vectors
        // From https://github.com/zcash-hackworks/zcash-test-vectors/blob/master/ff1.py
        TestVector {
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 2,
            tweak: vec![],
            pt: vec![0; 88],
            ct: vec![
                0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1,
                0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
                1, 1, 1, 1,
            ],
            binary: Some(BinaryTestVector {
                pt: vec![
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                ],
                ct: vec![
                    0x90, 0xac, 0xee, 0x3f, 0x83, 0xcd, 0xe7, 0xae, 0x56, 0x22, 0xf3,
                ],
            }),
        },
        TestVector {
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 2,
            tweak: vec![],
            pt: vec![
                0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1,
                0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
                1, 1, 1, 1,
            ],
            ct: vec![
                1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
                0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1,
                1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1,
                1, 0, 0, 0,
            ],
            binary: Some(BinaryTestVector {
                pt: vec![
                    0x90, 0xac, 0xee, 0x3f, 0x83, 0xcd, 0xe7, 0xae, 0x56, 0x22, 0xf3,
                ],
                ct: vec![
                    0x5b, 0x8b, 0xf1, 0x20, 0xf3, 0x9b, 0xab, 0x85, 0x27, 0xea, 0x1b,
                ],
            }),
        },
        TestVector {
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 2,
            tweak: vec![],
            pt: vec![
                0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                0, 1, 0, 1,
            ],
            ct: vec![
                0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1,
                0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0,
                0, 1, 1, 0,
            ],
            binary: Some(BinaryTestVector {
                pt: vec![
                    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
                ],
                ct: vec![
                    0xf0, 0x82, 0xb7, 0xee, 0x8f, 0x29, 0xc0, 0x76, 0x91, 0xce, 0x64,
                ],
            }),
        },
        TestVector {
            aes: AesType::AES256,
            key: vec![
                0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF,
                0x4F, 0x3C, 0xEF, 0x43, 0x59, 0xD8, 0xD5, 0x80, 0xAA, 0x4F, 0x7F, 0x03, 0x6D, 0x6F,
                0x04, 0xFC, 0x6A, 0x94,
            ],
            radix: 2,
            tweak: vec![
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
                65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104,
                105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120,
                121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
                137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
                153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
                169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184,
                185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
                201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216,
                217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
                233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
                249, 250, 251, 252, 253, 254,
            ],
            pt: vec![
                0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
                0, 1, 0, 1,
            ],
            ct: vec![
                0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0,
                0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1,
                1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0,
                0, 0, 1, 1,
            ],
            binary: Some(BinaryTestVector {
                pt: vec![
                    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
                ],
                ct: vec![
                    0xbe, 0x11, 0xb8, 0x86, 0xa8, 0x5, 0x9c, 0x27, 0x51, 0x7b, 0xc5,
                ],
            }),
        },
        // Specific test cases
        TestVector {
            aes: AesType::AES256,
            key: vec![0; 32],
            radix: 2,
            tweak: vec![],
            pt: vec![
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0,
            ],
            ct: vec![
                1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1,
                1, 0, 0, 0,
            ],
            binary: Some(BinaryTestVector {
                pt: vec![0x00, 0x00, 0x00, 0x00],
                ct: vec![0x7b, 0xf9, 0x4, 0x1b],
            }),
        },
    ])
}
