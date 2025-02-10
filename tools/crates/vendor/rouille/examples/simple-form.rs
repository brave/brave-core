#[macro_use]
extern crate rouille;

use std::io;

fn main() {
    // This example demonstrates how to handle HTML forms.

    // Note that like all examples we only listen on `localhost`, so you can't access this server
    // from another machine than your own.
    println!("Now listening on localhost:8000");

    rouille::start_server("localhost:8000", move |request| {
        rouille::log(&request, io::stdout(), || {
            router!(request,
                (GET) (/) => {
                    // When viewing the home page, we return an HTML document described below.
                    rouille::Response::html(FORM)
                },

                (POST) (/submit) => {
                    // This is the route that is called when the user submits the form of the
                    // home page.

                    // We query the data with the `post_input!` macro. Each field of the macro
                    // corresponds to an element of the form.
                    // If the macro returns an error (for example if a field is missing, which
                    // can happen if you screw up the form or if the user made a manual request)
                    // we return a 400 response.
                    let data = try_or_400!(post_input!(request, {
                        txt: String,
                        files: Vec<rouille::input::post::BufferedFile>,
                    }));

                    // We just print what was received on stdout. Of course in a real application
                    // you probably want to process the data, eg. store it in a database.
                    println!("Received data: {:?}", data);

                    rouille::Response::html("Success! <a href=\"/\">Go back</a>.")
                },

                _ => rouille::Response::empty_404()
            )
        })
    });
}

// The HTML document of the home page.
static FORM: &'static str = r#"
<html>
    <head>
        <title>Form</title>
    </head>
    <body>
        <form action="submit" method="POST" enctype="multipart/form-data">
            <p><input type="text" name="txt" placeholder="Some text" /></p>

            <p><input type="file" name="files" multiple /></p>

            <p><button>Upload</button></p>
        </form>
    </body>
</html>
"#;
