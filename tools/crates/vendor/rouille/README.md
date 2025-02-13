# Rouille, a Rust web micro-framework

Rouille is a micro-web-framework library. It creates a listening socket and parses incoming HTTP
requests from clients, then gives you the hand to process the request.

Rouille was designed to be intuitive to use if you know Rust. Contrary to express-like frameworks,
it doesn't employ middlewares. Instead everything is handled in a linear way.

Concepts closely related to websites (like cookies, CGI, form input, etc.) are directly supported
by rouille. More general concepts (like database handling or templating) are not directly handled,
as they are considered orthogonal to the micro web framework. However rouille's design makes it easy
to use in conjunction with any third-party library without the need for any glue code.

## [Documentation](https://docs.rs/rouille)

[![](https://docs.rs/rouille/badge.svg)](https://docs.rs/rouille)

## Getting started

If you have general knowledge about how HTTP works, [the documentation](https://docs.rs/rouille)
and [the well-documented examples](https://github.com/tomaka/rouille/tree/master/examples) are
good resources to get you started.

## License

Licensed under either of
 * Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)
at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you shall be dual licensed as above, without any
additional terms or conditions.

## FAQ

### What about performances?

Async I/O, green threads, coroutines, etc. in Rust are still very immature.

The rouille library just ignores this optimization and focuses on providing an easy-to-use
synchronous API instead, where each request is handled in its own dedicated thread.

Even if rouille itself was asynchronous, you would need asynchronous database clients and
asynchronous file loading in order to take advantage of it. There are currently no such libraries
in the Rust ecosystem.

Once async I/O has been figured out, rouille will be (hopefully transparently) updated to take it
into account.

### But is it fast?

On the author's old Linux machine, some basic benchmarking with `wrk -t 4 -c 4` shows the
following results:

- The hello-world example of rouille yields ~22k requests/sec.
- A hello world in nodejs (with `http.createServer`) yields ~14k requests/sec.
- The hello-world example of [tokio-minihttp](https://github.com/tokio-rs/tokio-minihttp) (which is
  supposedly the fastest HTTP server that currently exists) yields ~77k requests/sec. 
- The hello example of [hyper](https://github.com/hyperium/hyper) (which uses async I/O with mio
  as well) yields ~53k requests/sec.
- A hello world in Go yields ~51k requests/sec.
- The default installation of nginx yields ~39k requests/sec.

While not the fastest, rouille has *reasonable* performances. Amongst all these examples, rouille
is the only one to use synchronous I/O.

### Are there plugins for features such as database connection, templating, etc.

It should be trivial to integrate a database or templates to your web server written with
rouille. Moreover plugins need maintenance and tend to create a dependency hell. In the author's
opinion it is generally better not to use plugins.

### But I'm used to express-like frameworks!

Instead of doing this: (pseudo-code)

```js
server.add_middleware(function() {
    // middleware 1
});

server.add_middleware(function() {
    // middleware 2
});

server.add_middleware(function() {
    // middleware 3
});
```

In rouille you just handle each request entirely manually:

```rust
// initialize everything here

rouille::start_server(..., move |request| {
    // middleware 1

    // middleware 2

    // middleware 3
});
```
