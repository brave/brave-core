// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

extern crate rouille;

fn main() {
    // This example shows how to create a reverse proxy with rouille.

    println!("Now listening on localhost:8000");

    rouille::start_server("localhost:8000", move |request| {
        rouille::proxy::full_proxy(
            &request,
            rouille::proxy::ProxyConfig {
                addr: "example.com:80",
                replace_host: Some("example.com".into()),
            },
        )
        .unwrap()
    });
}
