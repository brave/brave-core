use float8::F8E4M3;

#[test]
fn test_all_close() {
    for n in 0..100 {
        let inp = (n as f64) * 0.1;
        let num = F8E4M3::from_f64(inp);
        // println!("out = {}, inp = {}", num.to_f16(), inp);
        println!("diff = {:.3}, inp = {inp}", num.to_f16().to_f64() - inp);
    }
}

#[test]
fn test_cmp() {
    let mut v: Vec<F8E4M3> = vec![
        F8E4M3::ONE,
        F8E4M3::INFINITY,
        F8E4M3::NEG_INFINITY,
        F8E4M3::NAN,
        F8E4M3::MAX_SUBNORMAL,
        -F8E4M3::MAX_SUBNORMAL,
        F8E4M3::ZERO,
        F8E4M3::NEG_ZERO,
        F8E4M3::NEG_ONE,
        F8E4M3::MIN_POSITIVE,
    ];

    v.sort_by(|a, b| a.total_cmp(b));

    assert!(v
        .into_iter()
        .zip(
            [
                F8E4M3::NEG_INFINITY,
                F8E4M3::NEG_ONE,
                -F8E4M3::MAX_SUBNORMAL,
                F8E4M3::NEG_ZERO,
                F8E4M3::ZERO,
                F8E4M3::MAX_SUBNORMAL,
                F8E4M3::MIN_POSITIVE,
                F8E4M3::ONE,
                F8E4M3::INFINITY,
                F8E4M3::NAN
            ]
            .iter()
        )
        .all(|(a, b)| a.to_bits() == b.to_bits()));
}
