use toml::map::Map;
use toml::Value::{Array, Boolean, Float, Integer, String, Table};

macro_rules! map( ($($k:expr => $v:expr),*) => ({
    let mut _m = Map::new();
    $(_m.insert($k.to_owned(), $v);)*
    _m
}) );

#[test]
fn simple_show() {
    assert_eq!(String("foo".to_owned()).to_string(), "\"foo\"");
    assert_eq!(Integer(10).to_string(), "10");
    assert_eq!(Float(10.0).to_string(), "10.0");
    assert_eq!(Float(2.4).to_string(), "2.4");
    assert_eq!(Boolean(true).to_string(), "true");
    assert_eq!(Array(vec![]).to_string(), "[]");
    assert_eq!(Array(vec![Integer(1), Integer(2)]).to_string(), "[1, 2]");
}

#[test]
fn table() {
    assert_eq!(map! {}.to_string(), "");
    assert_eq!(
        map! {
        "test" => Integer(2),
        "test2" => Integer(3) }
        .to_string(),
        "test = 2\ntest2 = 3\n"
    );
    assert_eq!(
        map! {
             "test" => Integer(2),
             "test2" => Table(map! {
                 "test" => String("wut".to_owned())
             })
        }
        .to_string(),
        "test = 2\n\
         \n\
         [test2]\n\
         test = \"wut\"\n"
    );
    assert_eq!(
        map! {
             "test" => Integer(2),
             "test2" => Table(map! {
                 "test" => String("wut".to_owned())
             })
        }
        .to_string(),
        "test = 2\n\
         \n\
         [test2]\n\
         test = \"wut\"\n"
    );
    assert_eq!(
        map! {
             "test" => Integer(2),
             "test2" => Array(vec![Table(map! {
                 "test" => String("wut".to_owned())
             })])
        }
        .to_string(),
        "test = 2\n\
         \n\
         [[test2]]\n\
         test = \"wut\"\n"
    );
    #[cfg(feature = "preserve_order")]
    assert_eq!(
        map! {
             "foo.bar" => Integer(2),
             "foo\"bar" => Integer(2)
        }
        .to_string(),
        "\"foo.bar\" = 2\n\
         'foo\"bar' = 2\n"
    );
    assert_eq!(
        map! {
             "test" => Integer(2),
             "test2" => Array(vec![Table(map! {
                 "test" => Array(vec![Integer(2)])
             })])
        }
        .to_string(),
        "test = 2\n\
         \n\
         [[test2]]\n\
         test = [2]\n"
    );
    let table = map! {
        "test" => Integer(2),
        "test2" => Array(vec![Table(map! {
            "test" => Array(vec![Array(vec![Integer(2), Integer(3)]),
            Array(vec![String("foo".to_owned()), String("bar".to_owned())])])
        })])
    };
    assert_eq!(
        table.to_string(),
        "test = 2\n\
         \n\
         [[test2]]\n\
         test = [[2, 3], [\"foo\", \"bar\"]]\n"
    );
    assert_eq!(
        map! {
             "test" => Array(vec![Integer(2)]),
             "test2" => Integer(2)
        }
        .to_string(),
        "test = [2]\n\
         test2 = 2\n"
    );
}
