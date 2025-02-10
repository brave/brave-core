//! Core deserialization logic that deserializes toml content to [`Value`]

use crate::{
    error::{Error, ErrorKind},
    tokens::{Error as TokenError, Token, Tokenizer},
    value::{self, Key, Value, ValueInner},
    Span,
};
use smallvec::SmallVec;
use std::{
    borrow::Cow,
    collections::{btree_map::Entry, BTreeMap},
};

type DeStr<'de> = Cow<'de, str>;
type TablePair<'de> = (Key<'de>, Val<'de>);
type InlineVec<T> = SmallVec<[T; 5]>;

/// Parses a toml string into a [`ValueInner::Table`]
pub fn parse(s: &str) -> Result<Value<'_>, Error> {
    let mut de = Deserializer::new(s);

    let mut tables = de.tables()?;
    let table_indices = build_table_indices(&tables);
    let table_pindices = build_table_pindices(&tables);

    let root_ctx = Ctx {
        depth: 0,
        cur: 0,
        cur_parent: 0,
        table_indices: &table_indices,
        table_pindices: &table_pindices,
        de: &de,
        values: None,
        max: tables.len(),
    };

    let mut root = value::Table::new();
    deserialize_table(root_ctx, &mut tables, &mut root)?;

    Ok(Value::with_span(
        ValueInner::Table(root),
        Span::new(0, s.len()),
    ))
}

struct Deserializer<'a> {
    input: &'a str,
    tokens: Tokenizer<'a>,
}

struct Ctx<'de, 'b> {
    depth: usize,
    cur: usize,
    cur_parent: usize,
    max: usize,
    table_indices: &'b BTreeMap<InlineVec<DeStr<'de>>, Vec<usize>>,
    table_pindices: &'b BTreeMap<InlineVec<DeStr<'de>>, Vec<usize>>,
    de: &'b Deserializer<'de>,
    values: Option<Vec<TablePair<'de>>>,
    //array: bool,
}

impl<'de, 'b> Ctx<'de, 'b> {
    #[inline]
    fn error(&self, start: usize, end: Option<usize>, kind: ErrorKind) -> Error {
        self.de.error(start, end, kind)
    }
}

fn deserialize_table<'de, 'b>(
    mut ctx: Ctx<'de, 'b>,
    tables: &'b mut [Table<'de>],
    table: &mut value::Table<'de>,
) -> Result<usize, Error> {
    while ctx.cur_parent < ctx.max && ctx.cur < ctx.max {
        if let Some(values) = ctx.values.take() {
            for (key, val) in values {
                table_insert(table, key, val, ctx.de)?;
            }
        }

        let next_table = {
            let prefix_stripped: InlineVec<_> = tables[ctx.cur_parent].header[..ctx.depth]
                .iter()
                .map(|v| v.name.clone())
                .collect::<InlineVec<_>>();
            ctx.table_pindices
                .get(&prefix_stripped)
                .and_then(|entries| {
                    let start = entries.binary_search(&ctx.cur).unwrap_or_else(|v| v);
                    if start == entries.len() || entries[start] < ctx.cur {
                        return None;
                    }
                    entries[start..].iter().find_map(|i| {
                        let i = *i;
                        (i < ctx.max && tables[i].values.is_some()).then_some(i)
                    })
                })
        };

        let Some(pos) = next_table else {
            break;
        };

        ctx.cur = pos;

        // Test to see if we're duplicating our parent's table, and if so
        // then this is an error in the toml format
        if ctx.cur_parent != pos {
            let cur = &tables[pos];
            let parent = &tables[ctx.cur_parent];
            if parent.header == cur.header {
                let name = cur.header.iter().fold(String::new(), |mut s, k| {
                    if !s.is_empty() {
                        s.push('.');
                    }
                    s.push_str(&k.name);
                    s
                });

                let first = Span::new(parent.at, parent.end);

                return Err(ctx.error(
                    cur.at,
                    Some(cur.end),
                    ErrorKind::DuplicateTable { name, first },
                ));
            }

            // If we're here we know we should share the same prefix, and if
            // the longer table was defined first then we want to narrow
            // down our parent's length if possible to ensure that we catch
            // duplicate tables defined afterwards.
            let parent_len = parent.header.len();
            let cur_len = cur.header.len();
            if cur_len < parent_len {
                ctx.cur_parent = pos;
            }
        }

        let ttable = &mut tables[pos];

        // If we're not yet at the appropriate depth for this table then we
        // just next the next portion of its header and then continue
        // decoding.
        if ctx.depth != ttable.header.len() {
            let key = ttable.header[ctx.depth].clone();
            if let Some((k, _)) = table.get_key_value(&key) {
                return Err(ctx.error(
                    key.span.start,
                    Some(key.span.end),
                    ErrorKind::DuplicateKey {
                        key: key.name.to_string(),
                        first: k.span,
                    },
                ));
            }

            let array = ttable.array && ctx.depth == ttable.header.len() - 1;
            ctx.cur += 1;

            let cctx = Ctx {
                depth: ctx.depth + if array { 0 } else { 1 },
                max: ctx.max,
                cur: 0,
                cur_parent: pos,
                table_indices: ctx.table_indices,
                table_pindices: ctx.table_pindices,
                de: ctx.de,
                values: None, //array.then(|| ttable.values.take().unwrap()),
            };

            let value = if array {
                let mut arr = Vec::new();
                deserialize_array(cctx, tables, &mut arr)?;
                ValueInner::Array(arr)
            } else {
                let mut tab = value::Table::new();
                deserialize_table(cctx, tables, &mut tab)?;
                ValueInner::Table(tab)
            };

            table.insert(key, Value::new(value));
            continue;
        }

        // Rule out cases like:
        //
        //      [[foo.bar]]
        //      [[foo]]
        if ttable.array {
            return Err(ctx.error(ttable.at, Some(ttable.end), ErrorKind::RedefineAsArray));
        }

        ctx.values = ttable.values.take();
    }

    Ok(ctx.cur_parent)
}

fn to_value<'de>(val: Val<'de>, de: &Deserializer<'de>) -> Result<Value<'de>, Error> {
    let value = match val.e {
        E::String(s) => ValueInner::String(s),
        E::Boolean(b) => ValueInner::Boolean(b),
        E::Integer(i) => ValueInner::Integer(i),
        E::Float(f) => ValueInner::Float(f),
        E::Array(arr) => {
            let mut varr = Vec::new();
            for val in arr {
                varr.push(to_value(val, de)?);
            }
            ValueInner::Array(varr)
        }
        E::DottedTable(tab) | E::InlineTable(tab) => {
            let mut ntable = value::Table::new();

            for (k, v) in tab {
                table_insert(&mut ntable, k, v, de)?;
            }

            ValueInner::Table(ntable)
        }
    };

    Ok(Value::with_span(value, Span::new(val.start, val.end)))
}

fn table_insert<'de>(
    table: &mut value::Table<'de>,
    key: Key<'de>,
    val: Val<'de>,
    de: &Deserializer<'de>,
) -> Result<(), Error> {
    match table.entry(key.clone()) {
        Entry::Occupied(occ) => Err(de.error(
            key.span.start,
            Some(key.span.end),
            ErrorKind::DuplicateKey {
                key: key.name.to_string(),
                first: occ.key().span,
            },
        )),
        Entry::Vacant(vac) => {
            vac.insert(to_value(val, de)?);
            Ok(())
        }
    }
}

fn deserialize_array<'de, 'b>(
    mut ctx: Ctx<'de, 'b>,
    tables: &'b mut [Table<'de>],
    arr: &mut Vec<value::Value<'de>>,
) -> Result<usize, Error> {
    // if let Some(values) = ctx.values.take() {
    //     for (key, val) in values {
    //         //printc!(&ctx, "{} => {val:?}", key.name);
    //         arr.push(to_value(val, ctx.de)?);
    //     }
    // }

    while ctx.cur_parent < ctx.max {
        let header_stripped = tables[ctx.cur_parent]
            .header
            .iter()
            .map(|v| v.name.clone())
            .collect::<InlineVec<_>>();
        let start_idx = ctx.cur_parent + 1;
        let next = ctx
            .table_indices
            .get(&header_stripped)
            .and_then(|entries| {
                let start = entries.binary_search(&start_idx).unwrap_or_else(|v| v);
                if start == entries.len() || entries[start] < start_idx {
                    return None;
                }
                entries[start..]
                    .iter()
                    .filter_map(|i| if *i < ctx.max { Some(*i) } else { None })
                    .map(|i| (i, &tables[i]))
                    .find(|(_, table)| table.array)
                    .map(|p| p.0)
            })
            .unwrap_or(ctx.max);

        let actx = Ctx {
            values: Some(
                tables[ctx.cur_parent]
                    .values
                    .take()
                    .expect("no array values"),
            ),
            max: next,
            depth: ctx.depth + 1,
            cur: 0,
            cur_parent: ctx.cur_parent,
            table_indices: ctx.table_indices,
            table_pindices: ctx.table_pindices,
            de: ctx.de,
        };

        let mut table = value::Table::new();
        deserialize_table(actx, tables, &mut table)?;
        arr.push(Value::new(ValueInner::Table(table)));

        ctx.cur_parent = next;
    }

    Ok(ctx.cur_parent)
}

// Builds a datastructure that allows for efficient sublinear lookups. The
// returned BTreeMap contains a mapping from table header (like [a.b.c]) to list
// of tables with that precise name. The tables are being identified by their
// index in the passed slice. We use a list as the implementation uses this data
// structure for arrays as well as tables, so if any top level [[name]] array
// contains multiple entries, there are multiple entries in the list. The lookup
// is performed in the `SeqAccess` implementation of `MapVisitor`. The lists are
// ordered, which we exploit in the search code by using bisection.
fn build_table_indices<'de>(tables: &[Table<'de>]) -> BTreeMap<InlineVec<DeStr<'de>>, Vec<usize>> {
    let mut res = BTreeMap::new();
    for (i, table) in tables.iter().enumerate() {
        let header = table
            .header
            .iter()
            .map(|v| v.name.clone())
            .collect::<InlineVec<_>>();
        res.entry(header).or_insert_with(Vec::new).push(i);
    }
    res
}

// Builds a datastructure that allows for efficient sublinear lookups. The
// returned BTreeMap contains a mapping from table header (like [a.b.c]) to list
// of tables whose name at least starts with the specified name. So searching
// for [a.b] would give both [a.b.c.d] as well as [a.b.e]. The tables are being
// identified by their index in the passed slice.
//
// A list is used for two reasons: First, the implementation also stores arrays
// in the same data structure and any top level array of size 2 or greater
// creates multiple entries in the list with the same shared name. Second, there
// can be multiple tables sharing the same prefix.
//
// The lookup is performed in the `MapAccess` implementation of `MapVisitor`.
// The lists are ordered, which we exploit in the search code by using
// bisection.
fn build_table_pindices<'de>(tables: &[Table<'de>]) -> BTreeMap<InlineVec<DeStr<'de>>, Vec<usize>> {
    let mut res = BTreeMap::new();
    for (i, table) in tables.iter().enumerate() {
        let header = table
            .header
            .iter()
            .map(|v| v.name.clone())
            .collect::<InlineVec<_>>();
        for len in 0..=header.len() {
            res.entry(header[..len].into())
                .or_insert_with(Vec::new)
                .push(i);
        }
    }
    res
}

// fn headers_equal(hdr_a: &[Key<'_>], hdr_b: &[Key<'_>]) -> bool {
//     if hdr_a.len() != hdr_b.len() {
//         return false;
//     }
//     hdr_a.iter().zip(hdr_b.iter()).all(|(h1, h2)| h1.1 == h2.1)
// }

#[derive(Debug)]
struct Table<'de> {
    at: usize,
    end: usize,
    header: InlineVec<Key<'de>>,
    values: Option<Vec<TablePair<'de>>>,
    array: bool,
}

impl<'a> Deserializer<'a> {
    fn new(input: &'a str) -> Deserializer<'a> {
        Deserializer {
            tokens: Tokenizer::new(input),
            input,
        }
    }

    fn tables(&mut self) -> Result<Vec<Table<'a>>, Error> {
        let mut tables = Vec::new();
        let mut cur_table = Table {
            at: 0,
            end: 0,
            header: InlineVec::new(),
            values: None,
            array: false,
        };

        while let Some(line) = self.line()? {
            match line {
                Line::Table {
                    at,
                    end,
                    mut header,
                    array,
                } => {
                    if !cur_table.header.is_empty() || cur_table.values.is_some() {
                        tables.push(cur_table);
                    }
                    cur_table = Table {
                        at,
                        end,
                        header: InlineVec::new(),
                        values: Some(Vec::new()),
                        array,
                    };
                    while let Some(part) = header.next().map_err(|e| self.token_error(e))? {
                        cur_table.header.push(part);
                    }
                }
                Line::KeyValue(key, value) => {
                    if cur_table.values.is_none() {
                        cur_table.values = Some(Vec::new());
                    }
                    self.add_dotted_key(key, value, cur_table.values.as_mut().unwrap())?;
                }
            }
        }
        if !cur_table.header.is_empty() || cur_table.values.is_some() {
            tables.push(cur_table);
        }
        Ok(tables)
    }

    fn line(&mut self) -> Result<Option<Line<'a>>, Error> {
        loop {
            self.eat_whitespace();
            if self.eat_comment()? {
                continue;
            }
            if self.eat(Token::Newline)? {
                continue;
            }
            break;
        }

        match self.peek()? {
            Some((_, Token::LeftBracket)) => self.table_header().map(Some),
            Some(_) => self.key_value().map(Some),
            None => Ok(None),
        }
    }

    fn table_header(&mut self) -> Result<Line<'a>, Error> {
        let start = self.tokens.current();
        self.expect(Token::LeftBracket)?;
        let array = self.eat(Token::LeftBracket)?;
        let ret = Header::new(self.tokens.clone(), array);
        self.tokens.skip_to_newline();
        let end = self.tokens.current();
        Ok(Line::Table {
            at: start,
            end,
            header: ret,
            array,
        })
    }

    fn key_value(&mut self) -> Result<Line<'a>, Error> {
        let key = self.dotted_key()?;
        self.eat_whitespace();
        self.expect(Token::Equals)?;
        self.eat_whitespace();

        let value = self.value()?;
        self.eat_whitespace();
        if !self.eat_comment()? {
            self.eat_newline_or_eof()?;
        }

        Ok(Line::KeyValue(key, value))
    }

    fn value(&mut self) -> Result<Val<'a>, Error> {
        let at = self.tokens.current();
        let value = match self.next()? {
            Some((Span { start, end }, Token::String { val, .. })) => Val {
                e: E::String(val),
                start,
                end,
            },
            Some((Span { start, end }, Token::Keylike("true"))) => Val {
                e: E::Boolean(true),
                start,
                end,
            },
            Some((Span { start, end }, Token::Keylike("false"))) => Val {
                e: E::Boolean(false),
                start,
                end,
            },
            Some((span, Token::Keylike(key))) => self.parse_keylike(at, span, key)?,
            Some((span, Token::Plus)) => self.number_leading_plus(span)?,
            Some((Span { start, .. }, Token::LeftBrace)) => {
                self.inline_table().map(|(Span { end, .. }, table)| Val {
                    e: E::InlineTable(table),
                    start,
                    end,
                })?
            }
            Some((Span { start, .. }, Token::LeftBracket)) => {
                self.array().map(|(Span { end, .. }, array)| Val {
                    e: E::Array(array),
                    start,
                    end,
                })?
            }
            Some(token) => {
                return Err(self.error(
                    at,
                    Some(token.0.end),
                    ErrorKind::Wanted {
                        expected: "a value",
                        found: token.1.describe(),
                    },
                ));
            }
            None => return Err(self.eof()),
        };
        Ok(value)
    }

    fn parse_keylike(&mut self, at: usize, span: Span, key: &'a str) -> Result<Val<'a>, Error> {
        if key == "inf" || key == "nan" {
            return self.number(span, key);
        }

        let first_char = key.chars().next().expect("key should not be empty here");
        match first_char {
            '-' | '0'..='9' => self.number(span, key),
            _ => Err(self.error(at, Some(span.end), ErrorKind::UnquotedString)),
        }
    }

    fn number(&mut self, Span { start, end }: Span, s: &'a str) -> Result<Val<'a>, Error> {
        let to_integer = |f| Val {
            e: E::Integer(f),
            start,
            end,
        };
        if let Some(s) = s.strip_prefix("0x") {
            self.integer(s, 16).map(to_integer)
        } else if let Some(s) = s.strip_prefix("0o") {
            self.integer(s, 8).map(to_integer)
        } else if let Some(s) = s.strip_prefix("0b") {
            self.integer(s, 2).map(to_integer)
        } else if s.contains('e') || s.contains('E') {
            self.float(s, None).map(|f| Val {
                e: E::Float(f),
                start,
                end: self.tokens.current(),
            })
        } else if self.eat(Token::Period)? {
            let at = self.tokens.current();
            match self.next()? {
                Some((Span { .. }, Token::Keylike(after))) => {
                    self.float(s, Some(after)).map(|f| Val {
                        e: E::Float(f),
                        start,
                        end: self.tokens.current(),
                    })
                }
                _ => Err(self.error(at, Some(end), ErrorKind::InvalidNumber)),
            }
        } else if s == "inf" {
            Ok(Val {
                e: E::Float(f64::INFINITY),
                start,
                end,
            })
        } else if s == "-inf" {
            Ok(Val {
                e: E::Float(f64::NEG_INFINITY),
                start,
                end,
            })
        } else if s == "nan" {
            Ok(Val {
                e: E::Float(f64::NAN.copysign(1.0)),
                start,
                end,
            })
        } else if s == "-nan" {
            Ok(Val {
                e: E::Float(f64::NAN.copysign(-1.0)),
                start,
                end,
            })
        } else {
            self.integer(s, 10).map(to_integer)
        }
    }

    fn number_leading_plus(&mut self, Span { start, end }: Span) -> Result<Val<'a>, Error> {
        let start_token = self.tokens.current();
        match self.next()? {
            Some((Span { end, .. }, Token::Keylike(s))) => self.number(Span { start, end }, s),
            _ => Err(self.error(start_token, Some(end), ErrorKind::InvalidNumber)),
        }
    }

    fn integer(&self, s: &'a str, radix: u32) -> Result<i64, Error> {
        let allow_sign = radix == 10;
        let allow_leading_zeros = radix != 10;
        let (prefix, suffix) = self.parse_integer(s, allow_sign, allow_leading_zeros, radix)?;
        let start = self.tokens.substr_offset(s);
        if !suffix.is_empty() {
            return Err(self.error(start, Some(start + s.len()), ErrorKind::InvalidNumber));
        }
        i64::from_str_radix(prefix.replace('_', "").trim_start_matches('+'), radix)
            .map_err(|_e| self.error(start, Some(start + s.len()), ErrorKind::InvalidNumber))
    }

    fn parse_integer(
        &self,
        s: &'a str,
        allow_sign: bool,
        allow_leading_zeros: bool,
        radix: u32,
    ) -> Result<(&'a str, &'a str), Error> {
        let start = self.tokens.substr_offset(s);

        let mut first = true;
        let mut first_zero = false;
        let mut underscore = false;
        let mut end = s.len();
        let send = start + s.len();
        for (i, c) in s.char_indices() {
            let at = i + start;
            if i == 0 && (c == '+' || c == '-') && allow_sign {
                continue;
            }

            if c == '0' && first {
                first_zero = true;
            } else if c.is_digit(radix) {
                if !first && first_zero && !allow_leading_zeros {
                    return Err(self.error(at, Some(send), ErrorKind::InvalidNumber));
                }
                underscore = false;
            } else if c == '_' && first {
                return Err(self.error(at, Some(send), ErrorKind::InvalidNumber));
            } else if c == '_' && !underscore {
                underscore = true;
            } else {
                end = i;
                break;
            }
            first = false;
        }
        if first || underscore {
            return Err(self.error(start, Some(send), ErrorKind::InvalidNumber));
        }
        Ok((&s[..end], &s[end..]))
    }

    fn float(&mut self, s: &'a str, after_decimal: Option<&'a str>) -> Result<f64, Error> {
        let (integral, mut suffix) = self.parse_integer(s, true, false, 10)?;
        let start = self.tokens.substr_offset(integral);

        let mut fraction = None;
        if let Some(after) = after_decimal {
            if !suffix.is_empty() {
                return Err(self.error(start, Some(start + s.len()), ErrorKind::InvalidNumber));
            }
            let (a, b) = self.parse_integer(after, false, true, 10)?;
            fraction = Some(a);
            suffix = b;
        }

        let mut exponent = None;
        if suffix.starts_with('e') || suffix.starts_with('E') {
            let (a, b) = if suffix.len() == 1 {
                self.eat(Token::Plus)?;
                match self.next()? {
                    Some((_, Token::Keylike(s))) => self.parse_integer(s, false, true, 10)?,
                    _ => {
                        return Err(self.error(
                            start,
                            Some(start + s.len()),
                            ErrorKind::InvalidNumber,
                        ))
                    }
                }
            } else {
                self.parse_integer(&suffix[1..], true, true, 10)?
            };
            if !b.is_empty() {
                return Err(self.error(start, Some(start + s.len()), ErrorKind::InvalidNumber));
            }
            exponent = Some(a);
        } else if !suffix.is_empty() {
            return Err(self.error(start, Some(start + s.len()), ErrorKind::InvalidNumber));
        }

        let mut number = integral
            .trim_start_matches('+')
            .chars()
            .filter(|c| *c != '_')
            .collect::<String>();
        if let Some(fraction) = fraction {
            number.push('.');
            number.extend(fraction.chars().filter(|c| *c != '_'));
        }
        if let Some(exponent) = exponent {
            number.push('E');
            number.extend(exponent.chars().filter(|c| *c != '_'));
        }
        number
            .parse()
            .map_err(|_e| self.error(start, Some(start + s.len()), ErrorKind::InvalidNumber))
            .and_then(|n: f64| {
                if n.is_finite() {
                    Ok(n)
                } else {
                    Err(self.error(start, Some(start + s.len()), ErrorKind::InvalidNumber))
                }
            })
    }

    // TODO(#140): shouldn't buffer up this entire table in memory, it'd be
    // great to defer parsing everything until later.
    fn inline_table(&mut self) -> Result<(Span, Vec<TablePair<'a>>), Error> {
        let mut ret = Vec::new();
        self.eat_whitespace();
        if let Some(span) = self.eat_spanned(Token::RightBrace)? {
            return Ok((span, ret));
        }
        loop {
            let key = self.dotted_key()?;
            self.eat_whitespace();
            self.expect(Token::Equals)?;
            self.eat_whitespace();
            let value = self.value()?;
            self.add_dotted_key(key, value, &mut ret)?;

            self.eat_whitespace();
            if let Some(span) = self.eat_spanned(Token::RightBrace)? {
                return Ok((span, ret));
            }
            self.expect(Token::Comma)?;
            self.eat_whitespace();
        }
    }

    // TODO(#140): shouldn't buffer up this entire array in memory, it'd be
    // great to defer parsing everything until later.
    fn array(&mut self) -> Result<(Span, Vec<Val<'a>>), Error> {
        let mut ret = Vec::new();

        let intermediate = |me: &mut Deserializer<'_>| -> Result<(), Error> {
            loop {
                me.eat_whitespace();
                if !me.eat(Token::Newline)? && !me.eat_comment()? {
                    break;
                }
            }
            Ok(())
        };

        loop {
            intermediate(self)?;
            if let Some(span) = self.eat_spanned(Token::RightBracket)? {
                return Ok((span, ret));
            }
            let value = self.value()?;
            ret.push(value);
            intermediate(self)?;
            if !self.eat(Token::Comma)? {
                break;
            }
        }
        intermediate(self)?;
        let span = self.expect_spanned(Token::RightBracket)?;
        Ok((span, ret))
    }

    fn table_key(&mut self) -> Result<Key<'a>, Error> {
        self.tokens.table_key().map_err(|e| self.token_error(e))
    }

    fn dotted_key(&mut self) -> Result<Vec<Key<'a>>, Error> {
        let mut result = Vec::new();
        result.push(self.table_key()?);
        self.eat_whitespace();
        while self.eat(Token::Period)? {
            self.eat_whitespace();
            result.push(self.table_key()?);
            self.eat_whitespace();
        }
        Ok(result)
    }

    /// Stores a value in the appropriate hierarchical structure positioned based on the dotted key.
    ///
    /// Given the following definition: `multi.part.key = "value"`, `multi` and `part` are
    /// intermediate parts which are mapped to the relevant fields in the deserialized type's data
    /// hierarchy.
    ///
    /// # Parameters
    ///
    /// * `key_parts`: Each segment of the dotted key, e.g. `part.one` maps to
    ///                `vec![Cow::Borrowed("part"), Cow::Borrowed("one")].`
    /// * `value`: The parsed value.
    /// * `values`: The `Vec` to store the value in.
    fn add_dotted_key(
        &self,
        mut key_parts: Vec<Key<'a>>,
        value: Val<'a>,
        values: &mut Vec<TablePair<'a>>,
    ) -> Result<(), Error> {
        let key = key_parts.remove(0);
        if key_parts.is_empty() {
            values.push((key, value));
            return Ok(());
        }
        match values
            .iter_mut()
            .find(|&&mut (ref k, _)| k.name == key.name)
        {
            Some(&mut (
                _,
                Val {
                    e: E::DottedTable(ref mut v),
                    ..
                },
            )) => {
                return self.add_dotted_key(key_parts, value, v);
            }
            Some(&mut (ref first, _)) => {
                return Err(self.error(
                    key.span.start,
                    Some(value.end),
                    ErrorKind::DottedKeyInvalidType { first: first.span },
                ));
            }
            None => {}
        }
        // The start/end value is somewhat misleading here.
        let table_values = Val {
            e: E::DottedTable(Vec::new()),
            start: value.start,
            end: value.end,
        };
        values.push((key, table_values));
        let last_i = values.len() - 1;
        if let (
            _,
            Val {
                e: E::DottedTable(ref mut v),
                ..
            },
        ) = values[last_i]
        {
            self.add_dotted_key(key_parts, value, v)?;
        }
        Ok(())
    }

    fn eat_whitespace(&mut self) {
        self.tokens.eat_whitespace();
    }

    fn eat_comment(&mut self) -> Result<bool, Error> {
        self.tokens.eat_comment().map_err(|e| self.token_error(e))
    }

    fn eat_newline_or_eof(&mut self) -> Result<(), Error> {
        self.tokens
            .eat_newline_or_eof()
            .map_err(|e| self.token_error(e))
    }

    fn eat(&mut self, expected: Token<'a>) -> Result<bool, Error> {
        self.tokens.eat(expected).map_err(|e| self.token_error(e))
    }

    fn eat_spanned(&mut self, expected: Token<'a>) -> Result<Option<Span>, Error> {
        self.tokens
            .eat_spanned(expected)
            .map_err(|e| self.token_error(e))
    }

    fn expect(&mut self, expected: Token<'a>) -> Result<(), Error> {
        self.tokens
            .expect(expected)
            .map_err(|e| self.token_error(e))
    }

    fn expect_spanned(&mut self, expected: Token<'a>) -> Result<Span, Error> {
        self.tokens
            .expect_spanned(expected)
            .map_err(|e| self.token_error(e))
    }

    fn next(&mut self) -> Result<Option<(Span, Token<'a>)>, Error> {
        self.tokens.step().map_err(|e| self.token_error(e))
    }

    fn peek(&mut self) -> Result<Option<(Span, Token<'a>)>, Error> {
        self.tokens.peek().map_err(|e| self.token_error(e))
    }

    fn eof(&self) -> Error {
        self.error(self.input.len(), None, ErrorKind::UnexpectedEof)
    }

    fn token_error(&self, error: TokenError) -> Error {
        match error {
            TokenError::InvalidCharInString(at, ch) => {
                self.error(at, None, ErrorKind::InvalidCharInString(ch))
            }
            TokenError::InvalidEscape(at, ch) => self.error(at, None, ErrorKind::InvalidEscape(ch)),
            TokenError::InvalidEscapeValue(at, len, v) => {
                self.error(at, Some(at + len), ErrorKind::InvalidEscapeValue(v))
            }
            TokenError::InvalidHexEscape(at, ch) => {
                self.error(at, None, ErrorKind::InvalidHexEscape(ch))
            }
            TokenError::NewlineInString(at) => {
                self.error(at, None, ErrorKind::InvalidCharInString('\n'))
            }
            TokenError::Unexpected(at, ch) => self.error(at, None, ErrorKind::Unexpected(ch)),
            TokenError::UnterminatedString(at) => {
                self.error(at, None, ErrorKind::UnterminatedString)
            }
            TokenError::Wanted {
                at,
                expected,
                found,
            } => self.error(
                at,
                Some(at + found.len()),
                ErrorKind::Wanted { expected, found },
            ),
            TokenError::MultilineStringKey(at, end) => {
                self.error(at, Some(end), ErrorKind::MultilineStringKey)
            }
        }
    }

    fn error(&self, start: usize, end: Option<usize>, kind: ErrorKind) -> Error {
        let span = Span::new(start, end.unwrap_or(start + 1));
        let line_info = Some(self.to_linecol(start));
        Error {
            span,
            kind,
            line_info,
        }
    }

    /// Converts a byte offset from an error message to a (line, column) pair
    ///
    /// All indexes are 0-based.
    fn to_linecol(&self, offset: usize) -> (usize, usize) {
        let mut cur = 0;
        // Use split_terminator instead of lines so that if there is a `\r`, it
        // is included in the offset calculation. The `+1` values below account
        // for the `\n`.
        for (i, line) in self.input.split_terminator('\n').enumerate() {
            if cur + line.len() + 1 > offset {
                return (i, offset - cur);
            }
            cur += line.len() + 1;
        }
        (self.input.lines().count(), 0)
    }
}

// impl Error {
//     pub(crate) fn line_col(&self) -> Option<(usize, usize)> {
//         self.line.map(|line| (line, self.col))
//     }

//     fn from_kind(at: Option<usize>, kind: ErrorKind) -> Self {
//         Error {
//             kind,
//             line: None,
//             col: 0,
//             at,
//             message: String::new(),
//             key: Vec::new(),
//         }
//     }

//     fn custom(at: Option<usize>, s: String) -> Self {
//         Error {
//             kind: ErrorKind::Custom,
//             line: None,
//             col: 0,
//             at,
//             message: s,
//             key: Vec::new(),
//         }
//     }

//     pub(crate) fn add_key_context(&mut self, key: &str) {
//         self.key.insert(0, key.to_string());
//     }

//     fn fix_offset<F>(&mut self, f: F)
//     where
//         F: FnOnce() -> Option<usize>,
//     {
//         // An existing offset is always better positioned than anything we might
//         // want to add later.
//         if self.at.is_none() {
//             self.at = f();
//         }
//     }

//     fn fix_linecol<F>(&mut self, f: F)
//     where
//         F: FnOnce(usize) -> (usize, usize),
//     {
//         if let Some(at) = self.at {
//             let (line, col) = f(at);
//             self.line = Some(line);
//             self.col = col;
//         }
//     }
// }

impl std::convert::From<Error> for std::io::Error {
    fn from(e: Error) -> Self {
        std::io::Error::new(std::io::ErrorKind::InvalidData, e.to_string())
    }
}

enum Line<'a> {
    Table {
        at: usize,
        end: usize,
        header: Header<'a>,
        array: bool,
    },
    KeyValue(Vec<Key<'a>>, Val<'a>),
}

struct Header<'a> {
    first: bool,
    array: bool,
    tokens: Tokenizer<'a>,
}

impl<'a> Header<'a> {
    fn new(tokens: Tokenizer<'a>, array: bool) -> Header<'a> {
        Header {
            first: true,
            array,
            tokens,
        }
    }

    fn next(&mut self) -> Result<Option<Key<'a>>, TokenError> {
        self.tokens.eat_whitespace();

        if self.first || self.tokens.eat(Token::Period)? {
            self.first = false;
            self.tokens.eat_whitespace();
            self.tokens.table_key().map(Some)
        } else {
            self.tokens.expect(Token::RightBracket)?;
            if self.array {
                self.tokens.expect(Token::RightBracket)?;
            }

            self.tokens.eat_whitespace();
            if !self.tokens.eat_comment()? {
                self.tokens.eat_newline_or_eof()?;
            }
            Ok(None)
        }
    }
}

#[derive(Debug)]
struct Val<'a> {
    e: E<'a>,
    start: usize,
    end: usize,
}

#[derive(Debug)]
enum E<'a> {
    Integer(i64),
    Float(f64),
    Boolean(bool),
    String(DeStr<'a>),
    Array(Vec<Val<'a>>),
    InlineTable(Vec<TablePair<'a>>),
    DottedTable(Vec<TablePair<'a>>),
}

impl<'a> E<'a> {
    #[allow(dead_code)]
    fn type_name(&self) -> &'static str {
        match *self {
            E::String(..) => "string",
            E::Integer(..) => "integer",
            E::Float(..) => "float",
            E::Boolean(..) => "boolean",
            E::Array(..) => "array",
            E::InlineTable(..) => "inline table",
            E::DottedTable(..) => "dotted table",
        }
    }
}
