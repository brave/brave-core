// Copyright © 2018 Phil Booth
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License. You may obtain
// a copy of the License at:
//
// https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

use super::*;

#[test]
fn as_ref_str() {
    assert_eq!(AsRef::<str>::as_ref(&Bom::Null), "[not set]");
    assert_eq!(AsRef::<str>::as_ref(&Bom::Bocu1), "BOCU-1");
    assert_eq!(AsRef::<str>::as_ref(&Bom::Gb18030), "GB 18030");
    assert_eq!(AsRef::<str>::as_ref(&Bom::Scsu), "SCSU");
    assert_eq!(AsRef::<str>::as_ref(&Bom::UtfEbcdic), "UTF-EBCDIC");
    assert_eq!(AsRef::<str>::as_ref(&Bom::Utf1), "UTF-1");
    assert_eq!(AsRef::<str>::as_ref(&Bom::Utf7), "UTF-7");
    assert_eq!(AsRef::<str>::as_ref(&Bom::Utf8), "UTF-8");
    assert_eq!(AsRef::<str>::as_ref(&Bom::Utf16Be), "UTF-16 (big-endian)");
    assert_eq!(
        AsRef::<str>::as_ref(&Bom::Utf16Le),
        "UTF-16 (little-endian)"
    );
    assert_eq!(AsRef::<str>::as_ref(&Bom::Utf32Be), "UTF-32 (big-endian)");
    assert_eq!(
        AsRef::<str>::as_ref(&Bom::Utf32Le),
        "UTF-32 (little-endian)"
    );
}

#[test]
fn to_string() {
    assert_eq!(Bom::Null.to_string(), Bom::Null.as_ref());
}

#[test]
fn as_ref_arr() {
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Null), &[]);
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Bocu1), &[0xfb, 0xee, 0x28]);
    assert_eq!(
        AsRef::<[u8]>::as_ref(&Bom::Gb18030),
        &[0x84, 0x31, 0x95, 0x33]
    );
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Scsu), &[0x0e, 0xfe, 0xff]);
    assert_eq!(
        AsRef::<[u8]>::as_ref(&Bom::UtfEbcdic),
        &[0xdd, 0x73, 0x66, 0x73]
    );
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Utf1), &[0xf7, 0x64, 0x4c]);
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Utf7), &[0x2b, 0x2f, 0x76]);
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Utf8), &[0xef, 0xbb, 0xbf]);
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Utf16Be), &[0xfe, 0xff]);
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Utf16Le), &[0xff, 0xfe]);
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Utf32Be), &[0, 0, 0xfe, 0xff]);
    assert_eq!(AsRef::<[u8]>::as_ref(&Bom::Utf32Le), &[0xff, 0xfe, 0, 0]);
}

#[test]
fn len() {
    assert_eq!(Bom::Null.len(), 0);
    assert_eq!(Bom::Bocu1.len(), 3);
    assert_eq!(Bom::Gb18030.len(), 4);
    assert_eq!(Bom::Scsu.len(), 3);
    assert_eq!(Bom::UtfEbcdic.len(), 4);
    assert_eq!(Bom::Utf1.len(), 3);
    assert_eq!(Bom::Utf7.len(), 4);
    assert_eq!(Bom::Utf8.len(), 3);
    assert_eq!(Bom::Utf16Be.len(), 2);
    assert_eq!(Bom::Utf16Le.len(), 2);
    assert_eq!(Bom::Utf32Be.len(), 4);
    assert_eq!(Bom::Utf32Le.len(), 4);
}

#[test]
fn default() {
    assert_eq!(Bom::default(), Bom::Null);
}

#[test]
fn from_slice() {
    assert_bom(&[], Bom::Null);

    assert_bom(&[0, 0, 0xfe], Bom::Null);
    assert_bom(&[0, 0, 0xfe, 0xfe], Bom::Null);
    assert_bom(&[0, 0, 0xfe, 0xff], Bom::Utf32Be);
    assert_bom(&[0, 0, 0xfe, 0xff, 42], Bom::Utf32Be);

    assert_bom(&[0x0e, 0xff], Bom::Null);
    assert_bom(&[0x0e, 0xff, 0xfe], Bom::Null);
    assert_bom(&[0x0e, 0xfe, 0xff], Bom::Scsu);
    assert_bom(&[0x0e, 0xfe, 0xff, 42], Bom::Scsu);

    assert_bom(&[0x84, 0x31, 0x95], Bom::Null);
    assert_bom(&[0x84, 0x31, 0x95, 0x32], Bom::Null);
    assert_bom(&[0x84, 0x31, 0x95, 0x33], Bom::Gb18030);
    assert_bom(&[0x84, 0x31, 0x95, 0x33, 42], Bom::Gb18030);
    assert_bom(&[0x84, 0x31, 0x95, 0x34], Bom::Null);

    assert_bom(&[0x2b, 0x2f, 0x76], Bom::Null);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x37], Bom::Null);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x38], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x38, 42], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x39], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x39, 42], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x3a], Bom::Null);

    assert_bom(&[0x2b, 0x2f, 0x76, 0x2a], Bom::Null);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x2b], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x2b, 42], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x2c], Bom::Null);

    assert_bom(&[0x2b, 0x2f, 0x76, 0x2e], Bom::Null);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x2f], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x2f, 42], Bom::Utf7);
    assert_bom(&[0x2b, 0x2f, 0x76, 0x30], Bom::Null);

    assert_bom(&[0xdd, 0x73, 0x66], Bom::Null);
    assert_bom(&[0xdd, 0x73, 0x66, 0x72], Bom::Null);
    assert_bom(&[0xdd, 0x73, 0x66, 0x73], Bom::UtfEbcdic);
    assert_bom(&[0xdd, 0x73, 0x66, 0x73, 42], Bom::UtfEbcdic);
    assert_bom(&[0xdd, 0x73, 0x66, 0x74], Bom::Null);

    assert_bom(&[0xef, 0xbb], Bom::Null);
    assert_bom(&[0xef, 0xbb, 0xbe], Bom::Null);
    assert_bom(&[0xef, 0xbb, 0xbf], Bom::Utf8);
    assert_bom(&[0xef, 0xbb, 0xbf, 42], Bom::Utf8);
    assert_bom(&[0xef, 0xbb, 0xc0], Bom::Null);

    assert_bom(&[0xf7, 0x64], Bom::Null);
    assert_bom(&[0xf7, 0x64, 0x4b], Bom::Null);
    assert_bom(&[0xf7, 0x64, 0x4c], Bom::Utf1);
    assert_bom(&[0xf7, 0x64, 0x4c, 42], Bom::Utf1);
    assert_bom(&[0xf7, 0x64, 0x4d], Bom::Null);

    assert_bom(&[0xfb, 0xee], Bom::Null);
    assert_bom(&[0xfb, 0xee, 0x27], Bom::Null);
    assert_bom(&[0xfb, 0xee, 0x28], Bom::Bocu1);
    assert_bom(&[0xfb, 0xee, 0x28, 42], Bom::Bocu1);
    assert_bom(&[0xfb, 0xee, 0x29], Bom::Null);

    assert_bom(&[0xfe], Bom::Null);
    assert_bom(&[0xfe, 0xfe], Bom::Null);
    assert_bom(&[0xfe, 0xff], Bom::Utf16Be);
    assert_bom(&[0xfe, 0xff, 42], Bom::Utf16Be);

    assert_bom(&[0xff], Bom::Null);
    assert_bom(&[0xff, 0xfd], Bom::Null);
    assert_bom(&[0xff, 0xfe], Bom::Utf16Le);
    assert_bom(&[0xff, 0xfe, 42], Bom::Utf16Le);
    assert_bom(&[0xff, 0xff], Bom::Null);

    assert_bom(&[0xff, 0xfe, 0], Bom::Utf16Le);
    assert_bom(&[0xff, 0xfe, 0, 0], Bom::Utf32Le);
    assert_bom(&[0xff, 0xfe, 0, 0, 42], Bom::Utf32Le);
    assert_bom(&[0xff, 0xfe, 0, 1], Bom::Utf16Le);
    assert_bom(&[0xff, 0xfe, 0, 1, 42], Bom::Utf16Le);
}

#[test]
fn from_as_ref_arr() {
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Null), Bom::Null);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Bocu1), Bom::Bocu1);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Gb18030), Bom::Gb18030);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Scsu), Bom::Scsu);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::UtfEbcdic), Bom::UtfEbcdic);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Utf1), Bom::Utf1);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Utf8), Bom::Utf8);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Utf16Be), Bom::Utf16Be);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Utf16Le), Bom::Utf16Le);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Utf32Be), Bom::Utf32Be);
    assert_bom(AsRef::<[u8]>::as_ref(&Bom::Utf32Le), Bom::Utf32Le);
}

fn assert_bom(bytes: &[u8], expected: Bom) {
    assert_eq!(Bom::from(bytes), expected);
}

#[test]
fn from_file() {
    let mut file = File::open("fixtures/ascii.txt").unwrap();
    assert_eq!(Bom::from(&mut file), Bom::Null);

    let mut file = File::open("fixtures/utf16-le.txt").unwrap();
    assert_eq!(Bom::from(&mut file), Bom::Utf16Le);

    let mut file = File::open("fixtures/utf32-le.txt").unwrap();
    assert_eq!(Bom::from(&mut file), Bom::Utf32Le);
}

#[test]
fn from_path() -> Result<(), Error> {
    let bom: Bom = "fixtures/ascii.txt".parse()?;
    assert_eq!(bom, Bom::Null);

    let bom: Bom = "fixtures/utf16-le.txt".parse()?;
    assert_eq!(bom, Bom::Utf16Le);

    let bom: Bom = "fixtures/utf32-le.txt".parse()?;
    assert_eq!(bom, Bom::Utf32Le);
    Ok(())
}
