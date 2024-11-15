use toml_edit::DocumentMut;

fn main() {
    let args = libtest_mimic::Arguments::from_args();
    let tests = toml_test_data::invalid()
        .map(|case| {
            libtest_mimic::Trial::test(case.name.display().to_string(), || {
                let expect_path =
                    std::path::Path::new("tests/fixtures").join(case.name.with_extension("stderr"));
                let err = match run_case(case.fixture) {
                    Ok(()) => "".to_owned(),
                    Err(err) => err,
                };
                snapbox::assert_data_eq!(err, snapbox::Data::read_from(&expect_path, None).raw());
                Ok(())
            })
        })
        .collect();
    libtest_mimic::run(&args, tests).exit()
}

fn run_case(input: &[u8]) -> Result<(), String> {
    let raw = std::str::from_utf8(input).map_err(|e| e.to_string())?;
    let _ = raw.parse::<DocumentMut>().map_err(|e| e.to_string())?;
    Ok(())
}
