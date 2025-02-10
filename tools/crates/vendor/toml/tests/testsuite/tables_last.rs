use std::collections::HashMap;

use serde::Deserialize;
use serde::Serialize;

#[test]
fn always_works() {
    // Ensure this works without the removed "toml::ser::tables_last"
    #[derive(Serialize)]
    struct A {
        vals: HashMap<&'static str, Value>,
    }

    #[derive(Serialize)]
    #[serde(untagged)]
    enum Value {
        Map(HashMap<&'static str, &'static str>),
        Int(i32),
    }

    let mut a = A {
        vals: HashMap::new(),
    };
    a.vals.insert("foo", Value::Int(0));

    let mut sub = HashMap::new();
    sub.insert("foo", "bar");
    a.vals.insert("bar", Value::Map(sub));

    toml::to_string(&a).unwrap();
}

#[test]
fn vec_of_vec_issue_387() {
    #[derive(Deserialize, Serialize, Debug)]
    struct Glyph {
        components: Vec<Component>,
        contours: Vec<Contour>,
    }

    #[derive(Deserialize, Serialize, Debug)]
    struct Point {
        x: f64,
        y: f64,
        pt_type: String,
    }

    type Contour = Vec<Point>;

    #[derive(Deserialize, Serialize, Debug)]
    struct Component {
        base: String,
        transform: (f64, f64, f64, f64, f64, f64),
    }

    let comp1 = Component {
        base: "b".to_owned(),
        transform: (1.0, 0.0, 0.0, 1.0, 0.0, 0.0),
    };
    let comp2 = Component {
        base: "c".to_owned(),
        transform: (1.0, 0.0, 0.0, 1.0, 0.0, 0.0),
    };
    let components = vec![comp1, comp2];

    let contours = vec![
        vec![
            Point {
                x: 3.0,
                y: 4.0,
                pt_type: "line".to_owned(),
            },
            Point {
                x: 5.0,
                y: 6.0,
                pt_type: "line".to_owned(),
            },
        ],
        vec![
            Point {
                x: 0.0,
                y: 0.0,
                pt_type: "move".to_owned(),
            },
            Point {
                x: 7.0,
                y: 9.0,
                pt_type: "offcurve".to_owned(),
            },
            Point {
                x: 8.0,
                y: 10.0,
                pt_type: "offcurve".to_owned(),
            },
            Point {
                x: 11.0,
                y: 12.0,
                pt_type: "curve".to_owned(),
            },
        ],
    ];
    let g1 = Glyph {
        components,
        contours,
    };

    let s = toml::to_string_pretty(&g1).unwrap();
    let _g2: Glyph = toml::from_str(&s).unwrap();
}

#[test]
fn vec_order_issue_356() {
    #[derive(Serialize, Deserialize)]
    struct Outer {
        v1: Vec<Inner>,
        v2: Vec<Inner>,
    }

    #[derive(Serialize, Deserialize)]
    struct Inner {}

    let outer = Outer {
        v1: vec![Inner {}],
        v2: vec![],
    };
    let s = toml::to_string_pretty(&outer).unwrap();
    let _o: Outer = toml::from_str(&s).unwrap();
}

#[test]
fn values_before_tables_issue_403() {
    #[derive(Serialize, Deserialize)]
    struct A {
        a: String,
        b: String,
    }

    #[derive(Serialize, Deserialize)]
    struct B {
        a: String,
        b: Vec<String>,
    }

    #[derive(Serialize, Deserialize)]
    struct C {
        a: A,
        b: Vec<String>,
        c: Vec<B>,
    }
    toml::to_string(&C {
        a: A {
            a: "aa".to_owned(),
            b: "ab".to_owned(),
        },
        b: vec!["b".to_owned()],
        c: vec![B {
            a: "cba".to_owned(),
            b: vec!["cbb".to_owned()],
        }],
    })
    .unwrap();
}
