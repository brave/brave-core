extern crate getopts;
extern crate scraper;

use getopts::Options;
use scraper::{Html, Selector};
use std::env;
use std::fs::{self, File};
use std::io::{self, Read};
use std::path::PathBuf;
use std::process;

enum Input {
    Document,
    Fragment,
}

enum Output {
    Html,
    InnerHtml,
    Attr(String),
    Classes,
    Id,
    Name,
    Text,
}

fn query<T: Read>(input: &Input, output: &Output, selector: &Selector, file: &mut T) -> bool {
    let mut html = String::new();
    file.read_to_string(&mut html).unwrap();

    let html = match *input {
        Input::Document => Html::parse_document(&html),
        Input::Fragment => Html::parse_fragment(&html),
    };

    let mut matched = false;
    for element in html.select(selector) {
        use crate::Output::*;
        match *output {
            Html => println!("{}", element.html()),
            InnerHtml => println!("{}", element.inner_html()),
            Attr(ref attr) => println!("{}", element.value().attr(attr).unwrap_or("")),
            Classes => println!(
                "{}",
                element.value().classes().collect::<Vec<_>>().join(" ")
            ),
            Id => println!("{}", element.value().id().unwrap_or("")),
            Name => println!("{}", element.value().name()),
            Text => println!("{}", element.text().collect::<String>()),
        }
        matched = true;
    }
    matched
}

fn main() {
    let mut opts = Options::new();
    opts.optflag("H", "html", "output HTML of elements");
    opts.optflag("I", "inner-html", "output inner HTML of elements");
    opts.optopt("a", "attr", "output attribute value of elements", "ATTR");
    opts.optflag("c", "classes", "output classes of elements");
    opts.optflag("d", "document", "parse input as HTML documents");
    opts.optflag("f", "fragment", "parse input as HTML fragments");
    opts.optflag("i", "id", "output ID of elements");
    opts.optflag("n", "name", "output name of elements");
    opts.optflag("t", "text", "output text of elements");
    opts.optflag("h", "help", "this cruft");
    opts.optopt("", "install-man-page", "install real documentation", "PATH");

    let args: Vec<String> = env::args().collect();
    let matches = match opts.parse(&args[1..]) {
        Ok(m) => m,
        Err(f) => panic!("{}", f.to_string()),
    };
    if matches.opt_present("h") {
        print!(
            "{}",
            opts.usage("Usage: scraper [options] SELECTOR [FILE ...]")
        );
        return;
    }

    if let Some(path) = matches.opt_str("install-man-page") {
        let mut path = PathBuf::from(path);
        if !path.ends_with("man1") {
            path.push("man1");
        }
        fs::create_dir_all(&path).unwrap();
        path.push("scraper.1");
        fs::write(&path, &include_bytes!("../scraper.1")[..]).unwrap();
        return;
    }

    let input = if matches.opt_present("f") {
        Input::Fragment
    } else {
        Input::Document
    };

    let output = if matches.opt_present("I") {
        Output::InnerHtml
    } else if matches.opt_present("a") {
        Output::Attr(matches.opt_str("a").unwrap())
    } else if matches.opt_present("c") {
        Output::Classes
    } else if matches.opt_present("i") {
        Output::Id
    } else if matches.opt_present("n") {
        Output::Name
    } else if matches.opt_present("t") {
        Output::Text
    } else {
        Output::Html
    };

    let selector = matches.free.first().expect("missing selector");
    let files = &matches.free[1..];

    let selector = Selector::parse(selector).unwrap();

    let matched = if files.is_empty() {
        query(&input, &output, &selector, &mut io::stdin())
    } else {
        files
            .iter()
            .map(File::open)
            .map(Result::unwrap)
            .any(|mut f| query(&input, &output, &selector, &mut f))
    };

    process::exit(i32::from(!matched));
}
