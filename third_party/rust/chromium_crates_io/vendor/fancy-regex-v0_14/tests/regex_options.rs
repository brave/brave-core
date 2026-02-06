use fancy_regex::RegexBuilder;

#[test]
fn check_casing_option() {
    let builder = RegexBuilder::new(r"TEST foo")
        .case_insensitive(false)
        .build();

    match builder {
        Ok(regex) => {
            assert!(regex.is_match(r"TEST foo").unwrap_or_default());
            assert!(!regex.is_match(r"test foo").unwrap_or_default());
        }
        _ => panic!("builder should be able to compile with casing options"),
    }
}

#[test]
fn check_override_casing_option() {
    let builder = RegexBuilder::new(r"FOO(?i:bar)quux")
        .case_insensitive(false)
        .build();

    match builder {
        Ok(regex) => {
            assert!(!regex.is_match("FoObarQuUx").unwrap_or_default());
            assert!(!regex.is_match("fooBARquux").unwrap_or_default());
            assert!(regex.is_match("FOObarquux").unwrap_or_default());
        }
        _ => panic!("builder should be able to compile with casing options"),
    }
}

#[test]
fn check_casing_insensitive_option() {
    let builder = RegexBuilder::new(r"TEST FOO")
        .case_insensitive(true)
        .build();

    match builder {
        Ok(regex) => assert!(regex.is_match(r"test foo").unwrap_or_default()),
        _ => panic!("builder should be able to compile with casing options"),
    }
}
