// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

extern crate rouille;

use rouille::cgi::CgiRun;
use std::process::Command;

fn main() {
    rouille::start_server("localhost:8000", move |request| {
        // TODO: add logging
        let mut cmd = Command::new("php-cgi");
        cmd.arg("-n"); // Don't use a php.ini.
        cmd.env("SCRIPT_FILENAME", "examples/php-test.php"); // The PHP script to use.
        cmd.env("REDIRECT_STATUS", "1"); // Necessary for security.
        cmd.start_cgi(&request).unwrap()
    });
}
