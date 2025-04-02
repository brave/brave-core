use cssparser::Token;

pub(crate) fn render_token(token: &Token<'_>) -> String {
    match token {
        Token::Ident(ident) => ident.to_string(),
        Token::AtKeyword(value) => format!("@{}", value),
        Token::Hash(name) | Token::IDHash(name) => format!("#{}", name),
        Token::QuotedString(value) => format!("\"{}\"", value),
        Token::UnquotedUrl(value) => value.to_string(),
        Token::Number {
            has_sign: signed,
            value: num,
            int_value: _,
        }
        | Token::Percentage {
            has_sign: signed,
            unit_value: num,
            int_value: _,
        } => render_number(*signed, *num, token),
        Token::Dimension {
            has_sign: signed,
            value: num,
            int_value: _,
            unit,
        } => format!("{}{}", render_int(*signed, *num), unit),
        Token::WhiteSpace(_) => String::from(" "),
        Token::Comment(comment) => format!("/* {} */", comment),
        Token::Function(name) => format!("{}()", name),
        Token::BadString(string) => format!("<Bad String {:?}>", string),
        Token::BadUrl(url) => format!("<Bad URL {:?}>", url),
        // Single-character token
        Token::Colon => ":".into(),
        Token::Semicolon => ";".into(),
        Token::Comma => ",".into(),
        Token::IncludeMatch => "~=".into(),
        Token::DashMatch => "|=".into(),
        Token::PrefixMatch => "^=".into(),
        Token::SuffixMatch => "$=".into(),
        Token::SubstringMatch => "*=".into(),
        Token::CDO => "<!--".into(),
        Token::CDC => "-->".into(),
        Token::ParenthesisBlock => "<(".into(),
        Token::SquareBracketBlock => "<[".into(),
        Token::CurlyBracketBlock => "<{".into(),
        Token::CloseParenthesis => "<)".into(),
        Token::CloseSquareBracket => "<]".into(),
        Token::CloseCurlyBracket => "<}".into(),
        Token::Delim(delim) => (*delim).into(),
    }
}

fn render_number(signed: bool, num: f32, token: &Token) -> String {
    let num = render_int(signed, num);

    match token {
        Token::Number { .. } => num,
        Token::Percentage { .. } => format!("{}%", num),
        _ => panic!("render_number is not supposed to be called on a non-numerical token"),
    }
}

fn render_int(signed: bool, num: f32) -> String {
    if signed {
        render_int_signed(num)
    } else {
        render_int_unsigned(num)
    }
}

fn render_int_signed(num: f32) -> String {
    if num > 0.0 {
        format!("+{}", num)
    } else {
        format!("-{}", num)
    }
}

fn render_int_unsigned(num: f32) -> String {
    format!("{}", num)
}

#[cfg(test)]
mod tests {
    use crate::Selector;

    #[test]
    fn regression_test_issue212() {
        let err = Selector::parse("div138293@!#@!!@#").unwrap_err();
        assert_eq!(err.to_string(), "Token \"@\" was not expected");
    }
}
