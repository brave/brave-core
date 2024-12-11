use std::fs;
use std::path::PathBuf;

/// Load a test file as bytes
pub(crate) fn fixture_as_raw(resource: &str) -> Vec<u8> {
    let path = fixture_filename(resource);
    fs::read(path).unwrap()
}

/// Load a test file and return it as a String
pub(crate) fn fixture_as_string(resource: &str) -> String {
    let path = fixture_filename(resource);
    fs::read_to_string(path).unwrap()
}

/// Return the path to our test data directory
pub(crate) fn fixture_dir() -> PathBuf {
    let mut dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    dir.push("fixture");
    dir
}

/// Return the path to a file within the test data directory
pub(crate) fn fixture_filename(filename: &str) -> String {
    let mut dir = fixture_dir();
    dir.push(filename);
    dir.to_str().unwrap().to_owned()
}
