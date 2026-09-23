fn main() {
    let mut args = std::env::args();
    let _ = args.next();
    let method = args.next().unwrap_or_else(|| String::from("from_ref"));
    let sample = args.next().unwrap_or_else(|| String::from("0123456789"));
    let count = args
        .next()
        .unwrap_or_else(|| String::from("10000000"))
        .parse::<usize>()
        .unwrap();
    #[allow(clippy::redundant_closure)] // Needed for consistent type
    let method = match method.as_str() {
        "from_ref" => |s| kstring::KString::from_ref(s),
        "from_string" => |s| kstring::KString::from_string(String::from(s)),
        _ => panic!("{:?} unsupported, try `from_ref`, `from_string`", method),
    };
    (0..count).map(|_| method(&sample)).last();
}
