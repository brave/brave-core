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
use std::env;
use std::io;
use std::process::Command;

fn main() {
    // This example demonstrates how to serve a git repository with rouille.
    // After starting this example, you should be able to run `git clone http://localhost:8000/`
    // in order to clone the repository of the current working directory.

    println!("Now listening on localhost:8000");

    rouille::start_server("localhost:8000", move |request| {
        rouille::log(&request, io::stdout(), || {
            // When a request is received, we invoke the `git http-backend` command through CGI.
            let mut cmd = Command::new("git");
            cmd.arg("http-backend");

            // We need to set some git-specific environment variables.
            cmd.env(
                "GIT_PROJECT_ROOT",
                env::current_dir().unwrap().to_str().unwrap(),
            );
            cmd.env("GIT_HTTP_EXPORT_ALL", ""); // This one is required to avoid security errors.

            // Our `cmd` is now ready. We can run it with the `start_cgi` method of the `CgiRun`
            // trait.
            // The `start_cgi` will add other CGI-specific environment variables, then feed stdin
            // and analyze stdout to build a rouille response.
            //
            // Note that an error is returned only if `git http-backend` fails to execute, and not
            // if the client sends bad data for example. In other words, an error can only occur
            // if the server was misconfigured. Therefore it's okay-ish to call `unwrap()` here.
            cmd.start_cgi(&request).unwrap()
        })
    });
}
