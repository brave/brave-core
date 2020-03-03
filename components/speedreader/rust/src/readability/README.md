readability-rs
=============

[![Build Status](https://travis-ci.org/kumabook/readability.svg?branch=master)](https://travis-ci.org/kumabook/readability)

readability-rs is a library for extracting the primary readable content of a webpage.
This is a rust port of arc90's readability project.
inspired by [kingwkb/readability](https://github.com/kingwkb/readability).


Hot to use
-------


- Add `readability` to dependencies in Cargo.toml

```toml
[dependencies]
readability = "^0"
```

- Then, use it as below

```rust

extern crate readability;
use readability::extractor;

fn main() {
  match extractor::scrape("https://spincoaster.com/chromeo-juice") {
      Ok(product) => {
          println!("------- html ------");
          println!("{}", product.content);
          println!("---- plain text ---");
          println!("{}", product.text);
      },
      Err(_) => println!("error occured"),
  }
}

```


Demo
-------

Visit [demo page](http://readability-rs.herokuapp.com/web/index.html).


Related Projects
----------------

- [Demo](https://github.com/kumabook/readability-demo)
- [ar90-readability ports](https://github.com/masukomi/ar90-readability#ports)

License
-------

[MIT](LICENSE)
