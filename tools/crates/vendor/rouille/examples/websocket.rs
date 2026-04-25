// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

#[macro_use]
extern crate rouille;

use std::thread;

use rouille::websocket;
use rouille::Response;

fn main() {
    // This example demonstrates how to use websockets with rouille.

    // Small message so that people don't need to read the source code.
    // Note that like all examples we only listen on `localhost`, so you can't access this server
    // from another machine than your own.
    println!("Now listening on localhost:8000");

    rouille::start_server("localhost:8000", move |request| {
        router!(request,
            (GET) (/) => {
                // The / route outputs an HTML client so that the user can try the websockets.
                // Note that in a real website you should probably use some templating system, or
                // at least load the HTML from a file.
                Response::html("<script type=\"text/javascript\">
                    var socket = new WebSocket(\"ws://localhost:8000/ws\", \"echo\");
                    function send(data) {{
                        socket.send(data);
                    }}
                    socket.onmessage = function(event) {{
                        document.getElementById('result').innerHTML += event.data + '<br />';
                    }}
                    </script>
                    <p>This example sends back everything you send to the server.</p>
                    <p><form onsubmit=\"send(document.getElementById('msg').value); return false;\">
                    <input type=\"text\" id=\"msg\" />
                    <button type=\"submit\">Send</button>
                    </form></p>
                    <p>Received: </p>
                    <p id=\"result\"></p>")
            },

            (GET) (/ws) => {
                // This is the websockets route.

                // In order to start using websockets we call `websocket::start`.
                // The function returns an error if the client didn't request websockets, in which
                // case we return an error 400 to the client thanks to the `try_or_400!` macro.
                //
                // The function returns a response to send back as part of the `start_server`
                // function, and a `websocket` variable of type `Receiver<Websocket>`.
                // Once the response has been sent back to the client, the `Receiver` will be
                // filled by rouille with a `Websocket` object representing the websocket.
                let (response, websocket) = try_or_400!(websocket::start(&request, Some("echo")));

                // Because of the nature of I/O in Rust, we need to spawn a separate thread for
                // each websocket.
                thread::spawn(move || {
                    // This line will block until the `response` above has been returned.
                    let ws = websocket.recv().unwrap();
                    // We use a separate function for better readability.
                    websocket_handling_thread(ws);
                });

                response
            },

            // Default 404 route as with all examples.
            _ => rouille::Response::empty_404()
        )
    });
}

// Function run in a separate thread.
fn websocket_handling_thread(mut websocket: websocket::Websocket) {
    // We wait for a new message to come from the websocket.
    while let Some(message) = websocket.next() {
        match message {
            websocket::Message::Text(txt) => {
                // If the message is text, send it back with `send_text`.
                println!("received {:?} from a websocket", txt);
                websocket.send_text(&txt).unwrap();
            }
            websocket::Message::Binary(_) => {
                println!("received binary from a websocket");
            }
        }
    }
}
