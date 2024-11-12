use ipld_core::{cid::Cid, ipld, ipld::Ipld};

#[test]
fn test_macro() {
    let _: Ipld = ipld!(null);
    let _: Ipld = ipld!(true);
    let _: Ipld = ipld!(false);
    let _: Ipld = ipld!(1);
    let _: Ipld = ipld!(1.0);
    let a: Ipld = ipld!("string");
    let _: Ipld = ipld!([]);
    let _: Ipld = ipld!([1, 2, 3]);
    let _: Ipld = ipld!({});
    let _: Ipld = ipld!({
        "bye": null,
        "numbers": [1, 2, 3],
        "a": a,
    });
    let cid = Cid::try_from("bafkreie74tgmnxqwojhtumgh5dzfj46gi4mynlfr7dmm7duwzyvnpw7h7m").unwrap();
    let _: Ipld = ipld!(cid);
}
