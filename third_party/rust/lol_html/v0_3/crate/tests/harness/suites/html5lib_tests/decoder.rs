use html5ever::data::{C1_REPLACEMENTS, NAMED_ENTITIES};
use lol_html::html_content::TextType;
use std::char;
use std::iter::Peekable;
use std::str::Chars;

pub fn to_null_decoded(s: &str) -> String {
    Decoder::new(s).unsafe_null().run()
}

pub fn decode_attr_value(s: &str) -> String {
    Decoder::new(s).unsafe_null().attr_entities().run()
}

pub fn decode_text(text: &str, text_type: TextType) -> String {
    let mut decoder = Decoder::new(text);

    if text_type.should_replace_unsafe_null_in_text() {
        decoder = decoder.unsafe_null();
    }

    if text_type.allows_html_entities() {
        decoder = decoder.text_entities();
    }

    decoder.run()
}

#[derive(PartialEq, Eq)]
enum Entities {
    None,
    Text,
    Attribute,
}

struct Decoder<'a> {
    chars: Peekable<Chars<'a>>,
    result: String,
    null: bool,
    entities: Entities,
}

impl<'a> Decoder<'a> {
    fn next_if_char(&mut self, expected: char) -> bool {
        self.next_if(|c| c == expected).is_some()
    }

    fn next_if(&mut self, f: impl Fn(char) -> bool) -> Option<char> {
        self.next_opt(|c| if f(c) { Some(c) } else { None })
    }

    fn next_opt<T>(&mut self, f: impl Fn(char) -> Option<T>) -> Option<T> {
        let opt = self.chars.peek().cloned().and_then(f);
        if opt.is_some() {
            self.chars.next();
        }
        opt
    }

    fn decode_numeric_entity(&mut self, radix: u32) -> bool {
        if let Some(mut code) = self.next_opt(|c| c.to_digit(radix)) {
            while let Some(digit) = self.next_opt(|c| c.to_digit(radix)) {
                if code < 0x0010_FFFF {
                    code = code * radix + digit;
                }
            }
            self.result.push(
                match code {
                    0x00 => None,
                    0x80..=0x9F => {
                        C1_REPLACEMENTS[(code - 0x80) as usize].or_else(|| char::from_u32(code))
                    }
                    _ => char::from_u32(code),
                }
                .unwrap_or('\u{FFFD}'),
            );
            self.next_if_char(';');
            true
        } else {
            self.result += "&#";
            false
        }
    }

    fn decode_named_entity(&mut self) {
        let mut name_buf = String::new();
        let mut name_match = ('&' as u32, 0, 0);
        while let Some(&c) = self.chars.peek() {
            name_buf.push(c);
            if let Some(&m) = NAMED_ENTITIES.get(&name_buf[..]) {
                self.chars.next();
                if m.0 != 0 {
                    if c != ';' && self.entities == Entities::Attribute {
                        if let Some(&c) = self.chars.peek() {
                            match c {
                                'A'..='Z' | 'a'..='z' | '0'..='9' | '=' => {
                                    continue;
                                }
                                _ => {}
                            }
                        }
                    }
                    name_match = (m.0, m.1, name_buf.len());
                }
            } else {
                name_buf.pop();
                break;
            }
        }
        self.result.push(char::from_u32(name_match.0).unwrap());
        if name_match.1 != 0 {
            self.result.push(char::from_u32(name_match.1).unwrap());
        }
        self.result += &name_buf[name_match.2..];
    }

    fn decode_entity(&mut self) {
        if self.next_if_char('#') {
            if let Some(x) = self.next_if(|c| c == 'x' || c == 'X') {
                if !self.decode_numeric_entity(16) {
                    self.result.push(x);
                }
            } else {
                self.decode_numeric_entity(10);
            }
        } else {
            self.decode_named_entity();
        }
    }

    fn decode_cr(&mut self) {
        self.result.push('\n');
        self.next_if_char('\n');
    }

    pub fn new(src: &'a str) -> Self {
        Decoder {
            chars: src.chars().peekable(),
            result: String::with_capacity(src.len()),
            null: false,
            entities: Entities::None,
        }
    }

    pub fn unsafe_null(mut self) -> Self {
        self.null = true;
        self
    }

    pub fn text_entities(mut self) -> Self {
        self.entities = Entities::Text;
        self
    }

    pub fn attr_entities(mut self) -> Self {
        self.entities = Entities::Attribute;
        self
    }

    pub fn run(mut self) -> String {
        while let Some(c) = self.chars.next() {
            match c {
                '\r' => {
                    self.decode_cr();
                }
                '\0' if self.null => {
                    self.result.push('\u{FFFD}');
                }
                '&' if self.entities != Entities::None => {
                    self.decode_entity();
                }
                _ => {
                    self.result.push(c);
                }
            }
        }

        self.result
    }
}
