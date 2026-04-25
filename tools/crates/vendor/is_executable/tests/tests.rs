extern crate diff;
extern crate is_executable;

use is_executable::is_executable;

#[cfg(unix)]
mod unix {
    use std::env;
    use std::fs::File;
    use std::io::Read;
    use std::process::Command;

    use super::*;

    #[test]
    fn cargo_readme_up_to_date() {
        if env::var("CI").is_ok() {
            return;
        }

        println!("Checking that `cargo readme > README.md` is up to date...");

        let expected = Command::new("cargo")
            .arg("readme")
            .current_dir(env!("CARGO_MANIFEST_DIR"))
            .output()
            .expect("should run `cargo readme` OK")
            .stdout;
        let expected = String::from_utf8_lossy(&expected);

        let actual = {
            let mut file = File::open(concat!(env!("CARGO_MANIFEST_DIR"), "/README.md"))
                .expect("should open README.md file");
            let mut s = String::new();
            file.read_to_string(&mut s)
                .expect("should read contents of file to string");
            s
        };

        if actual != expected {
            println!();
            println!("+++ expected README.md");
            println!("--- actual README.md");
            for d in diff::lines(&expected, &actual) {
                match d {
                    diff::Result::Left(l) => println!("+{}", l),
                    diff::Result::Right(r) => println!("-{}", r),
                    diff::Result::Both(b, _) => println!(" {}", b),
                }
            }
            panic!("Run `cargo readme > README.md` to update README.md")
        }
    }

    #[test]
    fn executable() {
        assert!(is_executable("./tests/i_am_executable"));
    }
}

#[cfg(target_os = "windows")]
mod windows {
    use super::*;

    #[test]
    fn executable() {
        assert!(is_executable("C:\\Windows\\explorer.exe"));
    }

}

#[test]
fn not_executable() {
    assert!(!is_executable("./tests/i_am_not_executable"));
}

#[test]
fn non_existant() {
    assert!(!is_executable("./tests/this-file-does-not-exist"));
}
