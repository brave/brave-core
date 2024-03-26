//! A module to handle macro used for tests only
//!
//! Macro are used to work around lifetime issues when calling Message::from_reader
//! from other fn.
//! This is fine as this is not something that should be done in normal case

macro_rules! test_serialize {
    ($hex:expr, $msg:expr) => {
        let mut serialized = Vec::new();
        {
            let mut writer = Writer::new(&mut serialized);
            $msg.write_message(&mut writer).unwrap();
        }
        let serialized_hex = encode_hex(&serialized);
        let hex = encode_hex(&decode_hex($hex));
        assert_eq!(serialized_hex, hex);
    };
}

macro_rules! test_deserialize {
    ($hex:expr, $msg:expr, $name:ident) => {
        let bytes = decode_hex($hex);
        {
            let mut reader = BytesReader::from_bytes(&bytes);
            let parsed = $name::from_reader(&mut reader, &bytes).unwrap();
            assert!($msg.eq(&parsed));
        }
    };
}

macro_rules! test_serialize_deserialize_length_delimited {
    ($msg:expr, $name:ident) => {
        let mut serialized = Vec::new();
        {
            let mut writer = Writer::new(&mut serialized);
            writer.write_message($msg).unwrap();
        }
        let mut reader = BytesReader::from_bytes(&serialized);
        let parsed: $name = reader.read_message(&serialized).unwrap();
        assert!($msg.eq(&parsed));
    };
}

macro_rules! test_serialize_deserialize {
    ($hex:expr, $msg:expr, $name:ident) => {
        let expected_bytes = decode_hex($hex);
        assert_eq!(expected_bytes.len(), $msg.get_size() as usize);

        test_serialize!($hex, $msg);
        test_deserialize!($hex, $msg, $name);
        test_serialize_deserialize_length_delimited!($msg, $name);
    };
}

// pub fn test_serialize_deserialize_no_hex<M : Message + MessageStatic>(msg: &M) {
//     let serialized_bytes = msg.write_to_bytes().unwrap();
//     let parsed = parse_from_bytes::<M>(&serialized_bytes).unwrap();
//     assert!(*msg == parsed);
// }
