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

    writeln!(stdout, "HTML fragment:").unwrap();
    stdout.flush().unwrap();
    input.clear();
    stdin.read_to_string(&mut input).unwrap();
    let fragment = Html::parse_fragment(&input);

    println!("{:#?}", fragment);

    for node in fragment.select(&selector) {
        println!("{:?}", node.value());
    }
}
