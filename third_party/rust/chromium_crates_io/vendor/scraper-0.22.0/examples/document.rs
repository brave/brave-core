extern crate scraper;

use std::io::{self, Read, Write};

use scraper::{Html, Selector};

fn main() {
    let mut input = String::new();
    let mut stdout = io::stdout();
    let mut stdin = io::stdin();

    write!(stdout, "CSS selector: ").unwrap();
    stdout.flush().unwrap();
    stdin.read_line(&mut input).unwrap();
    let selector = Selector::parse(&input).unwrap();

    writeln!(stdout, "HTML document:").unwrap();
    stdout.flush().unwrap();
    input.clear();
    stdin.read_to_string(&mut input).unwrap();
    let document = Html::parse_document(&input);

    println!("{:#?}", document);

    for node in document.select(&selector) {
        println!("{:?}", node.value());
    }
}
