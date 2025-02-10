use toml_edit::DocumentMut;

macro_rules! float_inf_tests {
    ($ty:ty) => {{
        let document = r"
            # infinity
            sf1 = inf  # positive infinity
            sf2 = +inf # positive infinity
            sf3 = -inf # negative infinity

            # not a number
            sf4 = nan  # actual sNaN/qNaN encoding is implementation specific
            sf5 = +nan # same as `nan`
            sf6 = -nan # valid, actual encoding is implementation specific

            # zero
            sf7 = +0.0
            sf8 = -0.0
        ";

        let document = document.parse::<DocumentMut>().unwrap();
        let float = |k| document[k].as_float().unwrap();

        assert!(float("sf1").is_infinite());
        assert!(float("sf1").is_sign_positive());
        assert!(float("sf2").is_infinite());
        assert!(float("sf2").is_sign_positive());
        assert!(float("sf3").is_infinite());
        assert!(float("sf3").is_sign_negative());

        assert!(float("sf4").is_nan());
        assert!(float("sf4").is_sign_positive());
        assert!(float("sf5").is_nan());
        assert!(float("sf5").is_sign_positive());
        assert!(float("sf6").is_nan());
        assert!(float("sf6").is_sign_negative());

        assert_eq!(float("sf7"), 0.0);
        assert!(float("sf7").is_sign_positive());
        assert_eq!(float("sf8"), 0.0);
        assert!(float("sf8").is_sign_negative());

        let mut document = DocumentMut::new();
        document["sf4"] = toml_edit::value(f64::NAN.copysign(1.0));
        document["sf6"] = toml_edit::value(f64::NAN.copysign(-1.0));
        assert_eq!(
            document.to_string(),
            "\
sf4 = nan
sf6 = -nan
"
        );
    }};
}

#[test]
fn test_float() {
    float_inf_tests!(f32);
    float_inf_tests!(f64);
}
