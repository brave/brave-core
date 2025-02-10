use std::f64;

use toml::toml;

macro_rules! table {
    ($($key:expr => $value:expr,)*) => {{
        // https://github.com/rust-lang/rust/issues/60643
        #[allow(unused_mut)]
        let mut table = toml::value::Table::new();
        $(
            table.insert($key.to_owned(), $value.into());
        )*
        toml::Value::Table(table)
    }};
}

macro_rules! array {
    ($($element:expr,)*) => {{
        // https://github.com/rust-lang/rust/issues/60643
        #![allow(clippy::vec_init_then_push)]
        #[allow(unused_mut)]
        let mut array = toml::value::Array::new();
        $(
            array.push($element.into());
        )*
        toml::Value::Array(array)
    }};
}

macro_rules! datetime {
    ($s:tt) => {
        $s.parse::<toml::value::Datetime>().unwrap()
    };
}

#[test]
fn test_cargo_toml() {
    // Simple sanity check of:
    //
    //   - Ordinary tables
    //   - Inline tables
    //   - Inline arrays
    //   - String values
    //   - Table keys containing hyphen
    //   - Table headers containing hyphen
    let actual = toml! {
        [package]
        name = "toml"
        version = "0.4.5"
        authors = ["Alex Crichton <alex@alexcrichton.com>"]

        [badges]
        travis-ci = { repository = "alexcrichton/toml-rs" }

        [dependencies]
        serde = "1.0"

        [dev-dependencies]
        serde_derive = "1.0"
        serde_json = "1.0"
    };

    let expected = table! {
        "package" => table! {
            "name" => "toml".to_owned(),
            "version" => "0.4.5".to_owned(),
            "authors" => array! {
                "Alex Crichton <alex@alexcrichton.com>".to_owned(),
            },
        },
        "badges" => table! {
            "travis-ci" => table! {
                "repository" => "alexcrichton/toml-rs".to_owned(),
            },
        },
        "dependencies" => table! {
            "serde" => "1.0".to_owned(),
        },
        "dev-dependencies" => table! {
            "serde_derive" => "1.0".to_owned(),
            "serde_json" => "1.0".to_owned(),
        },
    };

    assert_eq!(toml::Value::Table(actual), expected);
}

#[test]
fn test_array() {
    // Copied from the TOML spec.
    let actual = toml! {
        [[fruit]]
        name = "apple"

        [fruit.physical]
        color = "red"
        shape = "round"

        [[fruit.variety]]
        name = "red delicious"

        [[fruit.variety]]
        name = "granny smith"

        [[fruit]]
        name = "banana"

        [[fruit.variety]]
        name = "plantain"
    };

    let expected = table! {
        "fruit" => array! {
            table! {
                "name" => "apple",
                "physical" => table! {
                    "color" => "red",
                    "shape" => "round",
                },
                "variety" => array! {
                    table! {
                        "name" => "red delicious",
                    },
                    table! {
                        "name" => "granny smith",
                    },
                },
            },
            table! {
                "name" => "banana",
                "variety" => array! {
                    table! {
                        "name" => "plantain",
                    },
                },
            },
        },
    };

    assert_eq!(toml::Value::Table(actual), expected);
}

#[test]
fn test_number() {
    #![allow(clippy::unusual_byte_groupings)] // Verify the macro with odd formatting

    let actual = toml! {
        positive = 1
        negative = -1
        table = { positive = 1, negative = -1 }
        array = [ 1, -1 ]
        neg_zero = -0
        pos_zero = +0
        float = 1.618

        sf1 = inf
        sf2 = +inf
        sf3 = -inf
        sf7 = +0.0
        sf8 = -0.0

        hex = 0xa_b_c
        oct = 0o755
        bin = 0b11010110
    };

    let expected = table! {
        "positive" => 1,
        "negative" => -1,
        "table" => table! {
            "positive" => 1,
            "negative" => -1,
        },
        "array" => array! {
            1,
            -1,
        },
        "neg_zero" => -0,
        "pos_zero" => 0,
        "float" => 1.618,
        "sf1" => f64::INFINITY,
        "sf2" => f64::INFINITY,
        "sf3" => f64::NEG_INFINITY,
        "sf7" => 0.0,
        "sf8" => -0.0,
        "hex" => 2748,
        "oct" => 493,
        "bin" => 214,
    };

    assert_eq!(toml::Value::Table(actual), expected);
}

#[test]
fn test_nan() {
    let actual = toml! {
        sf4 = nan
        sf5 = +nan
        sf6 = -nan
    };

    let sf4 = actual["sf4"].as_float().unwrap();
    assert!(sf4.is_nan());
    assert!(sf4.is_sign_positive());

    let sf5 = actual["sf5"].as_float().unwrap();
    assert!(sf5.is_nan());
    assert!(sf5.is_sign_positive());

    let sf6 = actual["sf6"].as_float().unwrap();
    assert!(sf6.is_nan());
    assert!(sf6.is_sign_negative());
}

#[test]
fn test_datetime() {
    let actual = toml! {
        // Copied from the TOML spec.
        odt1 = 1979-05-27T07:32:00Z
        odt2 = 1979-05-27T00:32:00-07:00
        odt3 = 1979-05-27T00:32:00.999999-07:00
        odt4 = 1979-05-27 07:32:00Z
        ldt1 = 1979-05-27T07:32:00
        ldt2 = 1979-05-27T00:32:00.999999
        ld1 = 1979-05-27
        lt1 = 07:32:00
        lt2 = 00:32:00.999999

        table = {
            odt1 = 1979-05-27T07:32:00Z,
            odt2 = 1979-05-27T00:32:00-07:00,
            odt3 = 1979-05-27T00:32:00.999999-07:00,
            odt4 = 1979-05-27 07:32:00Z,
            ldt1 = 1979-05-27T07:32:00,
            ldt2 = 1979-05-27T00:32:00.999999,
            ld1 = 1979-05-27,
            lt1 = 07:32:00,
            lt2 = 00:32:00.999999,
        }

        array = [
            1979-05-27T07:32:00Z,
            1979-05-27T00:32:00-07:00,
            1979-05-27T00:32:00.999999-07:00,
            1979-05-27 07:32:00Z,
            1979-05-27T07:32:00,
            1979-05-27T00:32:00.999999,
            1979-05-27,
            07:32:00,
            00:32:00.999999,
        ]
    };

    let expected = table! {
        "odt1" => datetime!("1979-05-27T07:32:00Z"),
        "odt2" => datetime!("1979-05-27T00:32:00-07:00"),
        "odt3" => datetime!("1979-05-27T00:32:00.999999-07:00"),
        "odt4" => datetime!("1979-05-27 07:32:00Z"),
        "ldt1" => datetime!("1979-05-27T07:32:00"),
        "ldt2" => datetime!("1979-05-27T00:32:00.999999"),
        "ld1" => datetime!("1979-05-27"),
        "lt1" => datetime!("07:32:00"),
        "lt2" => datetime!("00:32:00.999999"),

        "table" => table! {
            "odt1" => datetime!("1979-05-27T07:32:00Z"),
            "odt2" => datetime!("1979-05-27T00:32:00-07:00"),
            "odt3" => datetime!("1979-05-27T00:32:00.999999-07:00"),
            "odt4" => datetime!("1979-05-27 07:32:00Z"),
            "ldt1" => datetime!("1979-05-27T07:32:00"),
            "ldt2" => datetime!("1979-05-27T00:32:00.999999"),
            "ld1" => datetime!("1979-05-27"),
            "lt1" => datetime!("07:32:00"),
            "lt2" => datetime!("00:32:00.999999"),
        },

        "array" => array! {
            datetime!("1979-05-27T07:32:00Z"),
            datetime!("1979-05-27T00:32:00-07:00"),
            datetime!("1979-05-27T00:32:00.999999-07:00"),
            datetime!("1979-05-27 07:32:00Z"),
            datetime!("1979-05-27T07:32:00"),
            datetime!("1979-05-27T00:32:00.999999"),
            datetime!("1979-05-27"),
            datetime!("07:32:00"),
            datetime!("00:32:00.999999"),
        },
    };

    assert_eq!(toml::Value::Table(actual), expected);
}

// This test requires rustc >= 1.20.
#[test]
fn test_quoted_key() {
    let actual = toml! {
        "quoted" = true
        table = { "quoted" = true }

        [target."cfg(windows)".dependencies]
        winapi = "0.2.8"
    };

    let expected = table! {
        "quoted" => true,
        "table" => table! {
            "quoted" => true,
        },
        "target" => table! {
            "cfg(windows)" => table! {
                "dependencies" => table! {
                    "winapi" => "0.2.8",
                },
            },
        },
    };

    assert_eq!(toml::Value::Table(actual), expected);
}

#[test]
fn test_empty() {
    let actual = toml! {
        empty_inline_table = {}
        empty_inline_array = []

        [empty_table]

        [[empty_array]]
    };

    let expected = table! {
        "empty_inline_table" => table! {},
        "empty_inline_array" => array! {},
        "empty_table" => table! {},
        "empty_array" => array! {
            table! {},
        },
    };

    assert_eq!(toml::Value::Table(actual), expected);
}

#[test]
fn test_dotted_keys() {
    let actual = toml! {
        a.b = 123
        a.c = 1979-05-27T07:32:00Z
        [table]
        a.b.c = 1
        a  .  b  .  d = 2
        in = { type.name = "cat", type.color = "blue" }
    };

    let expected = table! {
        "a" => table! {
            "b" => 123,
            "c" => datetime!("1979-05-27T07:32:00Z"),
        },
        "table" => table! {
            "a" => table! {
                "b" => table! {
                    "c" => 1,
                    "d" => 2,
                },
            },
            "in" => table! {
                "type" => table! {
                    "name" => "cat",
                    "color" => "blue",
                },
            },
        },
    };

    assert_eq!(toml::Value::Table(actual), expected);
}
