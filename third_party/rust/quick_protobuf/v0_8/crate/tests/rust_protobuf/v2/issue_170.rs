#[test]
fn t() {
    use super::{
        issue_170_a::A, issue_170_b::B, issue_170_c::C, issue_170_common::Common, issue_170_d::D,
        issue_170_e::E,
    };

    let b = B {
        common: Some(Common {}),
    };

    let a = A {
        common: Some(Common {}),
        b: Some(b.clone()),
    };

    let c = C {
        a: Some(a.clone()),
        b: Some(b.clone()),
        common: Some(Common {}),
    };

    let d = D {
        a: Some(a.clone()),
        b: Some(b.clone()),
        c: Some(c.clone()),
        common: Some(Common {}),
    };

    let e = E {
        a: Some(a.clone()),
        b: Some(b.clone()),
        c: Some(c.clone()),
        d: Some(d.clone()),
        common: Some(Common {}),
    };
}
