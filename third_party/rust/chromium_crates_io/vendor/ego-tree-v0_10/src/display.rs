use std::fmt::Display;

/// Indentation token
#[derive(Debug)]
struct Token {
    /// Is followed by a brother
    siblings: bool,
    /// Is intermediate while printing children
    children: bool,
}

impl Display for Token {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let Token { siblings, children } = self;

        write!(
            f,
            "{}",
            match (siblings, children) {
                (true, true) => "│   ",
                (true, false) => "├── ",
                (false, true) => "    ",
                (false, false) => "└── ",
            }
        )
    }
}

impl Token {
    /// Create a new indentation token
    fn new(siblings: bool) -> Self {
        Token {
            siblings,
            children: false,
        }
    }

    /// Set children flag before starting displaying children
    fn set_children(&mut self) {
        self.children = true;
    }
}

/// Manages the state during the display operation
#[derive(Debug)]
pub struct Indentation {
    tokens: Vec<Token>,
    ignore_root: bool,
}

impl Display for Indentation {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let first: usize = if self.ignore_root { 1 } else { 0 };

        for token in &self.tokens[first..] {
            write!(f, "{token}")?;
        }

        Ok(())
    }
}

impl Indentation {
    /// Creates a new indentation handler
    pub fn new(ignore_root: bool) -> Self {
        Indentation {
            tokens: Vec::new(),
            ignore_root,
        }
    }

    /// Adds a new layer of indentation
    pub fn indent(&mut self, siblings: bool) -> &mut Self {
        // Setup children mode for previous tokens
        let len = self.tokens.len();
        if len > 0 {
            self.tokens[len - 1].set_children();
        }

        self.tokens.push(Token::new(siblings));
        self
    }

    /// Removes the last layer of indentation
    pub fn deindent(&mut self) -> &mut Self {
        self.tokens.pop();
        self
    }
}
