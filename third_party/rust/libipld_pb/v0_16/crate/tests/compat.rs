use libipld::{cid::Cid, ipld, prelude::Codec, Ipld, IpldCodec};

struct TestCase {
    name: &'static str,
    node: Ipld,
    expected_bytes: &'static str,
}

#[test]
fn test_compat_roundtrip() {
    // Cases based on
    // https://github.com/ipld/go-codec-dagpb/blob/master/compat_test.go

    let empty_links = Vec::<Ipld>::new();
    let data_some = vec![0, 1, 2, 3, 4];
    let acid = Cid::try_from(&[1, 85, 0, 5, 0, 1, 2, 3, 4][..]).unwrap();
    let zero_name = "";
    let some_name = "some name";
    let zero_tsize: u64 = 0;
    let large_tsize: u64 = 9007199254740991; // JavaScript Number.MAX_SAFE_INTEGER

    let cases = [
        TestCase {
            name: "Links zero",
            node: ipld!({ "Links": empty_links.clone() }),
            expected_bytes: "",
        },
        TestCase {
            name: "Data some Links zero",
            node: ipld!({
                "Links": empty_links,
                "Data": data_some
            }),
            expected_bytes: "0a050001020304",
        },
        TestCase {
            name: "Links Hash some",
            node: ipld!({
                "Links": vec![ipld!({ "Hash": acid})],
            }),
            expected_bytes: "120b0a09015500050001020304",
        },
        TestCase {
            name: "Links Hash some Name zero",
            node: ipld!({
                "Links": vec![ipld!({
                    "Hash": acid,
                    "Name": zero_name,
                })]
            }),
            expected_bytes: "120d0a090155000500010203041200",
        },
        TestCase {
            name: "Links Hash some Name some",
            node: ipld!({
                "Links": vec![ipld!({
                    "Hash": acid,
                    "Name": some_name,
                })]
            }),
            expected_bytes: "12160a090155000500010203041209736f6d65206e616d65",
        },
        TestCase {
            name: "Links Hash some Tsize zero",
            node: ipld!({
                "Links": vec![ipld!({
                    "Hash": acid,
                    "Tsize": zero_tsize,
                })]
            }),
            expected_bytes: "120d0a090155000500010203041800",
        },
        TestCase {
            name: "Links Hash some Tsize some",
            node: ipld!({
                "Links": vec![ipld!({
                    "Hash": acid,
                    "Tsize": large_tsize,
                })]
            }),
            expected_bytes: "12140a0901550005000102030418ffffffffffffff0f",
        },
    ];

    for case in cases {
        println!("case {}", case.name);
        let result = IpldCodec::DagPb.encode(&case.node).unwrap();
        assert_eq!(result, hex::decode(case.expected_bytes).unwrap());

        let ipld: Ipld = IpldCodec::DagPb.decode(&result).unwrap();
        assert_eq!(ipld, case.node);
    }
}
