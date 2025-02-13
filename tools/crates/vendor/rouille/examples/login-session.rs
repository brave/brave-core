#![allow(unreachable_code)]
#[macro_use]
extern crate rouille;

use rouille::Request;
use rouille::Response;
use std::collections::HashMap;
use std::io;
use std::sync::Mutex;

// This struct contains the data that we store on the server about each client.
#[derive(Debug, Clone)]
struct SessionData {
    login: String,
}

fn main() {
    // This example demonstrates how to create a website with a simple login form.

    // Small message so that people don't need to read the source code.
    // Note that like all examples we only listen on `localhost`, so you can't access this server
    // from another machine than your own.
    println!("Now listening on localhost:8000");

    // For the sake of the example, we are going to store the sessions data in a hashmap in memory.
    // This has the disadvantage that all the sessions are erased if the program reboots (for
    // example because of an update), and that if you start multiple processes of the same
    // application (for example for load balancing) then they won't share sessions.
    // Therefore in a real project you should store probably the sessions in a database of some
    // sort instead.
    //
    // We created a struct that contains the data that we store on the server for each session,
    // and a hashmap that associates each session ID with the data.
    let sessions_storage: Mutex<HashMap<String, SessionData>> = Mutex::new(HashMap::new());

    rouille::start_server("localhost:8000", move |request| {
        rouille::log(&request, io::stdout(), || {
            // We call `session::session` in order to assign a unique identifier to each client.
            // This identifier is tracked through a cookie that is automatically appended to the
            // response.
            //
            // The parameters of the function are the name of the cookie (here "SID") and the
            // duration of the session in seconds (here, one hour).
            rouille::session::session(request, "SID", 3600, |session| {
                // If the client already has an identifier from a previous request, we try to load
                // the existing session data. If we successfully load data from `sessions_storage`,
                // we make a copy of the data in order to avoid locking the session for too long.
                //
                // We thus obtain a `Option<SessionData>`.
                let mut session_data = if session.client_has_sid() {
                    if let Some(data) = sessions_storage.lock().unwrap().get(session.id()) {
                        Some(data.clone())
                    } else {
                        None
                    }
                } else {
                    None
                };

                // Use a separate function to actually handle the request, for readability.
                // We pass a mutable reference to the `Option<SessionData>` so that the function
                // is free to modify it.
                let response = handle_route(&request, &mut session_data);

                // Since the function call to `handle_route` can modify the session data, we have
                // to store it back in the `sessions_storage` when necessary.
                if let Some(d) = session_data {
                    sessions_storage
                        .lock()
                        .unwrap()
                        .insert(session.id().to_owned(), d);
                } else if session.client_has_sid() {
                    // If `handle_route` erased the content of the `Option`, we remove the session
                    // from the storage. This is only done if the client already has an identifier,
                    // otherwise calling `session.id()` will assign one.
                    sessions_storage.lock().unwrap().remove(session.id());
                }

                // During the whole handling of the request, the `sessions_storage` mutex was only
                // briefly locked twice. This shouldn't have a lot of influence on performances.

                response
            })
        })
    });
}

// This is the function that truly handles the routes.
//
// The `session_data` parameter holds what we know about the client. It can be modified by the
// body of this function. Keep in my mind that the way we designed `session_data` is appropriate
// for most situations but not all. If for example you want to keep track of the pages that the
// user visited, you should design it in another way, otherwise the data of some requests will
// overwrite the data of other requests.
fn handle_route(request: &Request, session_data: &mut Option<SessionData>) -> Response {
    // First we handle the routes that are always accessible and always the same, no matter whether
    // the user is logged in or not.
    router!(request,
        (POST) (/login) => {
            // This is the route that is called when the user wants to log in.

            // In order to retrieve what the user sent us through the <form>, we use the
            // `post_input!` macro. This macro returns an error (if a field is missing for example),
            // so we use the `try_or_400!` macro to handle any possible error.
            //
            // If the macro is successful, `data` is an instance of a struct that has one member
            // for each field that we indicated in the macro.
            let data = try_or_400!(post_input!(request, {
                login: String,
                password: String,
            }));

            // Just a small debug message for this example. You could also output something in the
            // logs in a real application.
            println!("Login attempt with login {:?} and password {:?}", data.login, data.password);

            // In this example all login attempts are successful in the password starts with the
            // letter 'b'. Of course in a real website you should check the credentials in a proper
            // way.
            if data.password.starts_with("b") {
                // Logging the user in is done by writing the content of `session_data`.
                //
                // A minor warning here: in this demo we store in memory directly the data that
                // the user gave us. This data is not to be trusted and could contain anything,
                // including an attempt at XSS. Storing in memory what the user gave us is not
                // wrong, but we have to take care not to interpret it as HTML data for example.
                *session_data = Some(SessionData { login: data.login });
                return Response::redirect_303("/");

            } else {
                // We return a dummy response to indicate that the login failed. In a real
                // application you should probably use some sort of HTML templating instead.
                return Response::html("Wrong login/password");
            }
        },

        (POST) (/logout) => {
            // This route is called when the user wants to log out.
            // We do so by simply erasing the content of `session_data`, which deletes the session.
            *session_data = None;

            // We return a dummy response to indicate what happened. In a real application you
            // should probably use some sort of HTML templating instead.
            return Response::html(r#"Logout successful.
                                     <a href="/">Click here to go to the home</a>"#);
        },

        _ => ()
    );

    // Now that we handled all the routes that are accessible in all circumstances, we check
    // that the user is logged in before proceeding.
    if let Some(session_data) = session_data.as_ref() {
        // Logged in.
        handle_route_logged_in(request, session_data)
    } else {
        // Not logged in.
        router!(request,
            (GET) (/) => {
                // When connecting to the root, show a login form.
                // Note that in a real website you should probably use some templating system, or
                // at least load the HTML from a file.
                Response::html(r#"
                    <p>Hint: in this example all passwords that start with the letter 'b'
                       (lowercase) are valid.</p>
                    <form action="/login" method="POST">
                        <input type="text" name="login" placeholder="Login" />
                        <input type="password" name="password" placeholder="Password" />
                        <button type="submit">Go!</button>
                    </form>
                    <p>Or you can try <a href="/private">going to the private area</a>
                       without logging in, but you will be redirected back here.</p>
                "#)
            },

            _ => {
                // If the user tries to access any other route, redirect them to the login form.
                //
                // You may wonder: if I want to make some parts of my site public and some other
                // parts private, should I put all my public routes here? The answer is no. The way
                // this example is structured is appropriate for a website that is entirely
                // private. Don't hesitate to structure it in a different way, for example by
                // having a function that is dedicated only to public routes.
                Response::redirect_303("/")
            }
        )
    }
}

// This function handles the routes that are accessible only if the user is logged in.
fn handle_route_logged_in(request: &Request, _session_data: &SessionData) -> Response {
    router!(request,
        (GET) (/) => {
            // Show some greetings with a dummy response.
            Response::html(r#"You are now logged in. If you close your tab and open it again,
                              you will still be logged in.<br />
                              <a href="/private">Click here for the private area</a>
                              <form action="/logout" method="POST">
                              <button>Logout</button></form>"#)
        },

        (GET) (/private) => {
            // This route is here to demonstrate that the client can go to `/private` only if
            // they are successfully logged in.
            Response::html(r#"You are in the private area! <a href="/">Go back</a>."#)
        },

        _ => Response::empty_404()
    )
}
