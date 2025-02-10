[![Build Status](https://travis-ci.org/fitzgen/is_executable.svg?branch=master)](https://travis-ci.org/fitzgen/is_executable)

# is_executable

Is there an executable file at the given path?

[![](https://docs.rs/is_executable/badge.svg)](https://docs.rs/is_executable/) [![](http://meritbadge.herokuapp.com/is_executable) ![](https://img.shields.io/crates/d/is_executable.png)](https://crates.io/crates/is_executable) [![Unix Build Status](https://travis-ci.org/fitzgen/is_executable.png?branch=master)](https://travis-ci.org/fitzgen/is_executable) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/fitzgen/is_executable?branch=master&svg=true)](https://ci.appveyor.com/project/fitzgen/is-executable)

A small helper function which determines whether or not the given path points to
an executable file. If there is no file at the given path, or the file is not
executable, then `false` is returned. When there is a file and the file is
executable, then `true` is returned.

This crate works on both unix-based operating systems (mac, linux, freebsd, etc.) and Windows.

The API comes in two flavors:

1. An extension trait to add an `is_executable` method on `std::path::Path`:

    ```rust
    use std::path::Path;

    use is_executable::IsExecutable;

    fn main() {
        let path = Path::new("some/path/to/a/file");

        // Determine if `path` is executable.
        if path.is_executable() {
            println!("The path is executable!");
        } else {
            println!("The path is _not_ executable!");
        }
    }
    ```

2. For convenience, a standalone `is_executable` function, which takes any
`AsRef<Path>`:

    ```rust
    use std::path::Path;

    use is_executable::is_executable;

    fn main() {
        let path = Path::new("some/path/to/a/file");

        // Determine if `path` is executable.
        if is_executable(&path) {
            println!("The path is executable!");
        } else {
            println!("The path is _not_ executable!");
        }
    }
    ```

License: Apache-2.0/MIT
