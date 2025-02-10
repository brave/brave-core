#[test]
#[cfg(not(feature = "unbounded"))]
fn array_recursion_limit() {
    let depths = [(1, true), (20, true), (300, false)];
    for (depth, is_ok) in depths {
        let input = format!("x={}{}", &"[".repeat(depth), &"]".repeat(depth));
        let document = input.parse::<toml_edit::Document>();
        assert_eq!(document.is_ok(), is_ok, "depth: {}", depth);
    }
}

#[test]
#[cfg(not(feature = "unbounded"))]
fn inline_table_recursion_limit() {
    let depths = [(1, true), (20, true), (300, false)];
    for (depth, is_ok) in depths {
        let input = format!("x={}true{}", &"{ x = ".repeat(depth), &"}".repeat(depth));
        let document = input.parse::<toml_edit::Document>();
        assert_eq!(document.is_ok(), is_ok, "depth: {}", depth);
    }
}

#[test]
#[cfg(not(feature = "unbounded"))]
fn table_key_recursion_limit() {
    let depths = [(1, true), (20, true), (300, false)];
    for (depth, is_ok) in depths {
        let input = format!("[x{}]", &".x".repeat(depth));
        let document = input.parse::<toml_edit::Document>();
        assert_eq!(document.is_ok(), is_ok, "depth: {}", depth);
    }
}

#[test]
#[cfg(not(feature = "unbounded"))]
fn dotted_key_recursion_limit() {
    let depths = [(1, true), (20, true), (300, false)];
    for (depth, is_ok) in depths {
        let input = format!("x{} = true", &".x".repeat(depth));
        let document = input.parse::<toml_edit::Document>();
        assert_eq!(document.is_ok(), is_ok, "depth: {}", depth);
    }
}

#[test]
#[cfg(not(feature = "unbounded"))]
fn inline_dotted_key_recursion_limit() {
    let depths = [(1, true), (20, true), (300, false)];
    for (depth, is_ok) in depths {
        let input = format!("x = {{ x{} = true }}", &".x".repeat(depth));
        let document = input.parse::<toml_edit::Document>();
        assert_eq!(document.is_ok(), is_ok, "depth: {}", depth);
    }
}
