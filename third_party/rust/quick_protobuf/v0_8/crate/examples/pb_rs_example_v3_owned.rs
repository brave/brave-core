extern crate quick_protobuf;

mod owned {
    include!("pb_rs_v3/owned/mod.rs");
}

use std::borrow::Cow;
use std::convert::TryFrom;

use crate::owned::data_types::mod_FooMessage::OneOftest_oneof;
use crate::owned::data_types::{self, BazMessage, FooMessage, FooMessageOwned};

// Imported fields contain package a.b, which is translated into
// mod_a::mod_b rust module
use crate::owned::a::b::ImportedMessage;

use quick_protobuf::{MessageWrite, Writer};

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
        message
            .write_message(&mut writer)
            .expect("Cannot write message!");
    }
    println!("Message written successfully! bytes={:?}", &out);

    let read_message_owned = FooMessageOwned::try_from(out.clone()).unwrap();
    let read_message_owned = read_message_owned.proto();

    println!("{:?}", read_message_owned);
    assert_eq!(&message, read_message_owned);
    println!("Message read back and everything matches!");

    // Test mutability works too
    let mut read_message_owned_mut = FooMessageOwned::try_from(out).unwrap();
    let mut read_message_owned_mut = read_message_owned_mut.proto_mut();
    read_message_owned_mut.f_int32 += 1;
    assert_eq!(message.f_int32 + 1, read_message_owned_mut.f_int32);
    read_message_owned_mut.f_string = "I see you, too!".into();
    assert_eq!(
        read_message_owned_mut.f_string,
        Cow::Borrowed("I see you, too!")
    );
}
