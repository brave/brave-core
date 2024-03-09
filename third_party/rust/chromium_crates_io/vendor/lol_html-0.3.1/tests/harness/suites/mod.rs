use std::fs::File;
use std::io::BufReader;

fn data_dir_path(path: &str) -> String {
    format!("{}/tests/data/{}", env!("CARGO_MANIFEST_DIR"), path)
}

fn for_each_test_file(path: &str, handler: &mut dyn FnMut(BufReader<File>)) {
    glob::glob(&data_dir_path(path)).unwrap().for_each(|path| {
        handler(BufReader::new(File::open(path.unwrap()).unwrap()));
    });
}

fn get_test_file_reader(path: &str) -> BufReader<File> {
    BufReader::new(File::open(data_dir_path(path)).unwrap())
}

pub mod html5lib_tests;
pub mod selectors_tests;
