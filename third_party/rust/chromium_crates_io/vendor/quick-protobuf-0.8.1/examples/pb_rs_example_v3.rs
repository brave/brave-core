extern crate quick_protobuf;

mod pb_rs_v3;

use std::borrow::Cow;

use crate::pb_rs_v3::data_types::mod_FooMessage::OneOftest_oneof;
use crate::pb_rs_v3::data_types::{self, BarMessage, BazMessage, FooMessage, RepeatedMessage};

// Imported fields contain package a.b, which is translated into
// mod_a::mod_b rust module
use crate::pb_rs_v3::a::b::ImportedMessage;

use quick_protobuf::{deserialize_from_slice, serialize_into_vec, BytesReader, Writer};

fn main() {
    // Generate a message, somehow
    //
    // For the example we will leverage the `Default` derive of all messages
    let message = FooMessage {
        // Regular field work as expected, optional leverages on rust Option<>
        f_int32: 54,

        // strings are borrowed (Cow)
        f_string: Cow::Borrowed("Hello world from example!"),

        // bytes too
        f_bytes: Cow::Borrowed(b"I see you!"),

        // imported fields work as expected
        f_imported: Some(ImportedMessage { i: true }),

        // nested messages are encapsulated into a rust module mod_Message
        f_nested: Some(data_types::mod_BazMessage::Nested {
            f_nested: Some(data_types::mod_BazMessage::mod_Nested::NestedMessage { f_nested: 2 }),
        }),

        // nested enums too
        f_nested_enum: data_types::mod_BazMessage::mod_Nested::NestedEnum::Baz,

        // a map!
        f_map: vec![(Cow::Borrowed("foo"), 1), (Cow::Borrowed("bar"), 2)]
            .into_iter()
            .collect(),

        // a oneof value
        test_oneof: OneOftest_oneof::f1(2),

        f_repeated_string: vec![Cow::Borrowed("goat"), Cow::Borrowed("running")],
        f_repeated_baz_message: vec![BazMessage {
            nested: Some(data_types::mod_BazMessage::Nested {
                f_nested: Some(data_types::mod_BazMessage::mod_Nested::NestedMessage {
                    f_nested: 2,
                }),
            }),
            b_int64: 10,
            b_string: Cow::Borrowed("boom\n"),
        }],

        // Each message implements Default ... which makes it much easier
        ..FooMessage::default()
    };

    // Write the message into our buffer, it could be a file, a ZipFile etc ...
    let mut out = Vec::new();
    {
        let mut writer = Writer::new(&mut out);
        writer
            .write_message(&message)
            .expect("Cannot write message!");
    }
    println!("Message written successfully! bytes={:?}", &out);

    // alternatively, a one-liner if we want to work with vec directly
    let out_vec = serialize_into_vec(&message).expect("Cannot write message!");
    assert_eq!(out, out_vec);

    // Try to read it back and confirm that we still have the exact same message
    //
    // In general, if we had written the data to say, a file, or if someone else
    // have written that data, it would be more convenient to use a `Reader` which
    // will feed an internal, owned, buffer. Here, on the contrary, we already hold
    // the `out` buffer. Thus it is more efficient to directly use a `BytesWriter`.
    let read_message = {
        let mut reader = BytesReader::from_bytes(&out);
        reader
            .read_message::<FooMessage>(&out)
            .expect("Cannot read message")
    };
    assert_eq!(message, read_message);

    // alternatively, a one-liner if we want to work with slices directly
    let read_vec = deserialize_from_slice(&out).expect("Cannot write message!");
    assert_eq!(message, read_vec);

    println!("Message read back and everything matches!");
    println!("{:#?}", read_message);

    let message = RepeatedMessage {
        bar_message: vec![BarMessage { b_int32: 0 }, BarMessage { b_int32: 9 }],
    };

    let mut out = Vec::new();
    {
        let mut writer = Writer::new(&mut out);
        writer
            .write_message(&message)
            .expect("Cannot write message!");
    }
    println!("Message written successfully! bytes={:?}", &out);

    // alternatively, a one-liner if we want to work with vec directly
    let out_vec = serialize_into_vec(&message).expect("Cannot write message!");
    assert_eq!(out, out_vec);

    // Try to read it back and confirm that we still have the exact same message
    //
    // In general, if we had written the data to say, a file, or if someone else
    // have written that data, it would be more convenient to use a `Reader` which
    // will feed an internal, owned, buffer. Here, on the contrary, we already hold
    // the `out` buffer. Thus it is more efficient to directly use a `BytesWriter`.
    let read_message = {
        let mut reader = BytesReader::from_bytes(&out);
        reader
            .read_message::<RepeatedMessage>(&out)
            .expect("Cannot read message")
    };
    assert_eq!(message, read_message);

    // alternatively, a one-liner if we want to work with slices directly
    let read_vec = deserialize_from_slice(&out).expect("Cannot write message!");
    assert_eq!(message, read_vec);

    println!("Message read back and everything matches!");
    println!("{:#?}", read_message);
}
