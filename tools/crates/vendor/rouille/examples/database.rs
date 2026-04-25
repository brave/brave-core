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
extern crate postgres;
extern crate serde;
#[macro_use]
extern crate serde_derive;

use std::sync::Mutex;

use postgres::{Client, NoTls, Transaction};

use rouille::Request;
use rouille::Response;

fn main() {
    // This example demonstrates how to connect to a database and perform queries when the client
    // performs a request.
    // The server created in this example uses a REST API.

    // The first thing we do is try to connect to the database.
    //
    // One important thing to note here is that we wrap a `Mutex` around the connection. Since the
    // request handler can be called multiple times in parallel, everything that we use in it must
    // be thread-safe. By default the PostgresSQL connection isn't thread-safe, so we need a mutex
    // to make it thread-safe.
    //
    // Not wrapping a mutex around the database would lead to a compilation error when we attempt
    // to use the variable `db` from within the closure passed to `start_server`.
    let db = {
        let db = Client::connect("postgres://test:test@localhost/test", NoTls);
        Mutex::new(db.expect("Failed to connect to database"))
    };

    // We perform some initialization for the sake of the example.
    // In a real application you probably want to have a migrations system. This is out of scope
    // of rouille.
    {
        let sql = "CREATE TABLE IF NOT EXISTS notes (
                    id SERIAL PRIMARY KEY,
                    content TEXT NOT NULL
                   );";
        db.lock()
            .unwrap()
            .execute(sql, &[])
            .expect("Failed to initialize database");
    }

    // Small message so that people don't need to read the source code.
    // Note that like all the other examples, we only listen on `localhost`, so you can't access this server
    // from any machine other than your own.
    println!("Now listening on localhost:8000");

    // Now the server starts listening. The `move` keyword will ensure that we move the `db` variable
    // into the closure. Not putting `move` here would result in a compilation error.
    //
    // Note that in an ideal world, `move` wouldn't be necessary here. Unfortunately Rust isn't
    // smart enough yet to understand that the database can't be destroyed while we still use it.
    rouille::start_server("localhost:8000", move |request| {
        // Since we wrapped the database connection around a `Mutex`, we lock it here before usage.
        //
        // This will give us exclusive access to the database connection for the handling of this
        // request. Unfortunately the consequence is that if a request is made while another one
        // is already being processed, the second one will have to wait for the first one to
        // complete.
        //
        // In a real application you probably want to create multiple connections instead of just
        // one, and make each request use a different connection.
        //
        // In addition to this, if a panic happens while the `Mutex` is locked then the database
        // connection will likely be in a corrupted state and the next time the mutex is locked
        // it will panic. This is another good reason to use multiple connections.
        let mut db = db.lock().unwrap();

        // Start a transaction so that if a panic happens during the processing of the request,
        // any change made to the database will be rolled back.
        let mut db = db.transaction().unwrap();

        // For better readability, we handle the request in a separate function.
        let response = note_routes(&request, &mut db);

        // If the response is a success, we commit the transaction before returning. It's only at
        // this point that data are actually written in the database.
        if response.is_success() {
            db.commit().unwrap();
        }

        response
    });
}

// This function actually handles the request.
fn note_routes(request: &Request, db: &mut Transaction) -> Response {
    router!(request,
        (GET) (/) => {
            // For the sake of the example we just put a dummy route for `/` so that you see
            // something if you connect to the server with a browser.
            Response::text("Hello! Unfortunately there is nothing to see here.")
        },

        (GET) (/notes) => {
            // This route returns the list of notes. We perform the query and output it as JSON.

            #[derive(Serialize)]
            struct Elem { id: String }

            let mut out = Vec::new();
            // We perform the query and iterate over the rows, writing each row to `out`.
            for row in &db.query("SELECT id FROM notes", &[]).unwrap() {
                let id: i32 = row.get(0);
                out.push(Elem { id: format!("/note/{}", id) });
            }

            Response::json(&out)
        },

        (GET) (/note/{id: i32}) => {
            // This route returns the content of a note, if it exists.

            // Note that this code is a bit unergonomic, but this is mostly a problem with the
            // database client library and not rouille itself.

            // To do so, we first create a variable that will receive the content of the note.
            let mut content: Option<String> = None;
            // And then perform the query and write to `content`. This line can only panic if the
            // SQL is malformed.
            for row in &db.query("SELECT content FROM notes WHERE id = $1", &[&id]).unwrap() {
                content = Some(row.get(0));
            }

            // If `content` is still empty at this point, this means that the note doesn't
            // exist in the database. Otherwise, we return the content.
            match content {
                Some(content) => Response::text(content),
                None => Response::empty_404(),
            }
        },

        (PUT) (/note/{id: i32}) => {
            // This route modifies the content of an existing note.

            // We start by reading the body of the HTTP request into a `String`.
            let body = try_or_400!(rouille::input::plain_text_body(&request));

            // And write the content with a query. This line can only panic if the
            // SQL is malformed.
            let updated = db.execute("UPDATE notes SET content = $2 WHERE id = $1",
                                     &[&id, &body]).unwrap();

            // We determine whether the note existed based on the number of rows that
            // were modified.
            if updated >= 1 {
                Response::text("The note has been updated")
            } else {
                Response::empty_404()
            }
        },

        (POST) (/note) => {
            // This route creates a new note whose initial content is the body.

            // We start by reading the body of the HTTP request into a `String`.
            let body = try_or_400!(rouille::input::plain_text_body(&request));

            // To do so, we first create a variable that will receive the content.
            let mut id: Option<i32> = None;
            // And then perform the query and write to `content`. This line can only panic if the
            // SQL is malformed.
            for row in &db.query("INSERT INTO notes(content) VALUES ($1) RETURNING id", &[&body]).unwrap() {
                id = Some(row.get(0));
            }

            let id = id.unwrap();

            let mut response = Response::text("The note has been created");
            response.status_code = 201;
            response.headers.push(("Location".into(), format!("/note/{}", id).into()));
            response
        },

        (DELETE) (/note/{id: i32}) => {
            // This route deletes a note. This line can only panic if the
            // SQL is malformed.
            db.execute("DELETE FROM notes WHERE id = $1", &[&id]).unwrap();
            Response::text("")
        },

        // If none of the other blocks matches the request, return a 404 response.
        _ => Response::empty_404()
    )
}
