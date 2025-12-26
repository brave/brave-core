//! Core deserialization logic that deserializes toml content to [`Value`]

use crate::{
    Span,
    error::{Error, ErrorKind},
    tokens::{Error as TokenError, Token, Tokenizer},
    value::{self, Key, Value, ValueInner},
};
use smallvec::SmallVec;
use std::{
    borrow::Cow,
    collections::{BTreeMap, btree_map::Entry},
    ops::Range,
};

type DeStr<'de> = Cow<'de, str>;
type TablePair<'de> = (Key<'de>, Val<'de>);
type InlineVec<T> = SmallVec<[T; 5]>;

/// Parses a toml string into a [`ValueInner::Table`]
pub fn parse(s: &str) -> Result<Value<'_>, Error> {
    let mut de = Deserializer::new(s);

    let raw_tables = de.tables()?;
    let mut ctx = DeserializeCtx {
        table_indices: &build_table_indices(&raw_tables),
        table_pindices: &build_table_pindices(&raw_tables),
        raw_tables,
        de: &de,
    };
    let root = ctx.deserialize_entry(
        DeserializeTableIdx {
            table_idx: 0,
            depth: 0,
            idx_range: 0..ctx.raw_tables.len(),
        },
        Vec::new(),
    )?;

    Ok(Value::with_span(root, Span::new(0, s.len())))
}

struct Deserializer<'a> {
    input: &'a str,
    tokens: Tokenizer<'a>,
}

struct DeserializeCtx<'de, 'b> {
    raw_tables: Vec<Table<'de>>,
    // maps table headers to a list of tables with that exact header
    // (the list contains indices into `raw_tables` and is ordered)
    table_indices: &'b BTreeMap<InlineVec<DeStr<'de>>, Vec<usize>>,
    // maps table headers to a list of all subtables
    // (the list contains indices into `raw_tables` and is ordered)
    table_pindices: &'b BTreeMap<InlineVec<DeStr<'de>>, Vec<usize>>,
    de: &'b Deserializer<'de>,
}
// specifies the table/array that is currently being deserialized, namely the
// table/array with the header `raw_tables[table_idx].header[0..depth]`
struct DeserializeTableIdx {
    // index of the first occurence of the desired header (even as a prefix)
    table_idx: usize,
    depth: usize,
    // range of `raw_tables` indices to consider, used to isolate subtables of
    // different array entries
    idx_range: Range<usize>,
}
impl DeserializeTableIdx {
    fn get_header<'de>(&self, raw_tables: &[Table<'de>]) -> InlineVec<DeStr<'de>> {
        if self.depth == 0 {
            return InlineVec::new();
        }

        raw_tables[self.table_idx].header[0..self.depth]
            .iter()
            .map(|key| key.name.clone())
            .collect()
    }
}
impl<'de, 'b> DeserializeCtx<'de, 'b> {
    // deserialize the table/array given by `table_idx`
    fn deserialize_entry(
        &mut self,
        table_idx: DeserializeTableIdx,
        // values defined via dotted keys should be passed on to the corresponding subtable
        additional_values: Vec<TablePair<'de>>,
    ) -> Result<value::ValueInner<'de>, Error> {
        let current_header = table_idx.get_header(&self.raw_tables);
        let matching_tables = self.get_matching_tables(&current_header, &table_idx.idx_range);

        let is_array = matching_tables
            .iter()
            .all(|idx| self.raw_tables[*idx].array)
            && !matching_tables.is_empty();

        if is_array {
            // catch invalid cases like:
            //   [a.b]
            //   [[a]]
            if table_idx.table_idx < matching_tables[0] {
                let array_tbl = &self.raw_tables[matching_tables[0]];
                return Err(self.de.error(
                    array_tbl.at,
                    Some(array_tbl.end),
                    ErrorKind::RedefineAsArray,
                ));
            }
            assert!(additional_values.is_empty());

            let mut array = value::Array::new();
            for (i, array_entry_idx) in matching_tables.iter().copied().enumerate() {
                let entry_range_end = matching_tables
                    .get(i + 1)
                    .copied()
                    .unwrap_or(table_idx.idx_range.end);

                let span = Self::get_table_span(&self.raw_tables[array_entry_idx]);
                let values = self.raw_tables[array_entry_idx].values.take().unwrap();
                let array_entry = self.deserialize_as_table(
                    &current_header,
                    array_entry_idx..entry_range_end,
                    values.values.into_iter(),
                )?;
                array.push(Value::with_span(ValueInner::Table(array_entry), span));
            }
            Ok(ValueInner::Array(array))
        } else {
            if matching_tables.len() > 1 {
                let first_tbl = &self.raw_tables[matching_tables[0]];
                let second_tbl = &self.raw_tables[matching_tables[1]];
                return Err(self.de.error(
                    second_tbl.at,
                    Some(second_tbl.end),
                    ErrorKind::DuplicateTable {
                        name: current_header.last().unwrap().to_string(),
                        first: Span::new(first_tbl.at, first_tbl.end),
                    },
                ));
            }

            let values = matching_tables
                .first()
                .map(|idx| {
                    self.raw_tables[*idx]
                        .values
                        .take()
                        .unwrap()
                        .values
                        .into_iter()
                })
                .unwrap_or_default()
                .chain(additional_values);
            let subtable =
                self.deserialize_as_table(&current_header, table_idx.idx_range, values)?;

            Ok(ValueInner::Table(subtable))
        }
    }
    fn deserialize_as_table(
        &mut self,
        header: &[DeStr<'de>],
        range: Range<usize>,
        values: impl Iterator<Item = TablePair<'de>>,
    ) -> Result<value::Table<'de>, Error> {
        let mut table = value::Table::new();
        let mut dotted_keys_map = BTreeMap::new();

        for (key, val) in values {
            match val.e {
                E::DottedTable(mut tbl_vals) => {
                    tbl_vals.span = Some(Span::new(val.start, val.end));
                    dotted_keys_map.insert(key, tbl_vals);
                }
                _ => table_insert(&mut table, key, val, self.de)?,
            }
        }

        let subtables = self.get_subtables(header, &range);
        for &subtable_idx in subtables {
            if self.raw_tables[subtable_idx].values.is_none() {
                continue;
            }

            let subtable_name = &self.raw_tables[subtable_idx].header[header.len()];

            let dotted_entries = match dotted_keys_map.remove_entry(subtable_name) {
                // Detect redefinitions of tables created via dotted keys, as
                // these are considered errors, e.g:
                //   apple.color = "red"
                //   [apple]  # INVALID
                // However adding subtables is allowed:
                //   apple.color = "red"
                //   [apple.texture]  # VALID
                Some((previous_key, _))
                    if self.raw_tables[subtable_idx].header.len() == header.len() + 1 =>
                {
                    return Err(self.de.error(
                        subtable_name.span.start,
                        Some(subtable_name.span.end),
                        ErrorKind::DuplicateKey {
                            key: subtable_name.to_string(),
                            first: previous_key.span,
                        },
                    ));
                }
                Some((_, dotted_entries)) => dotted_entries.values,
                None => Vec::new(),
            };

            match table.entry(subtable_name.clone()) {
                Entry::Vacant(vac) => {
                    let subtable_span = Self::get_table_span(&self.raw_tables[subtable_idx]);
                    let subtable_idx = DeserializeTableIdx {
                        table_idx: subtable_idx,
                        depth: header.len() + 1,
                        idx_range: range.clone(),
                    };
                    let entry = self.deserialize_entry(subtable_idx, dotted_entries)?;
                    vac.insert(Value::with_span(entry, subtable_span));
                }
                Entry::Occupied(occ) => {
                    return Err(self.de.error(
                        subtable_name.span.start,
                        Some(subtable_name.span.end),
                        ErrorKind::DuplicateKey {
                            key: subtable_name.to_string(),
                            first: occ.key().span,
                        },
                    ));
                }
            };
        }

        for (key, val) in dotted_keys_map {
            let val_span = val.span.unwrap();
            let val = Val {
                e: E::DottedTable(val),
                start: val_span.start,
                end: val_span.end,
            };
            table_insert(&mut table, key, val, self.de)?;
        }

        Ok(table)
    }

    fn get_matching_tables(&self, header: &[DeStr<'de>], range: &Range<usize>) -> &'b [usize] {
        let matching_tables = self
            .table_indices
            .get(header)
            .map(Vec::as_slice)
            .unwrap_or_default();
        Self::get_subslice_in_range(matching_tables, range)
    }
    fn get_subtables(&self, header: &[DeStr<'de>], range: &Range<usize>) -> &'b [usize] {
        let subtables = self
            .table_pindices
            .get(header)
            .map(Vec::as_slice)
            .unwrap_or_default();
        Self::get_subslice_in_range(subtables, range)
    }
    fn get_subslice_in_range<'a>(slice: &'a [usize], range: &Range<usize>) -> &'a [usize] {
        let start_idx = slice.partition_point(|idx| *idx < range.start);
        let end_idx = slice.partition_point(|idx| *idx < range.end);
        &slice[start_idx..end_idx]
    }

    fn get_table_span(ttable: &Table<'de>) -> Span {
        ttable.values.as_ref().and_then(|v| v.span).map_or_else(
            || Span::new(ttable.at, ttable.end),
            |span| Span::new(ttable.at.min(span.start), ttable.end.max(span.end)),
        )
    }
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

            for (k, v) in tab.values {
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
// of tables whose name starts with the specified name and is strictly longer.
// So searching for [a.b] would give both [a.b.c.d] as well as [a.b.e], but not
// [a.b] itself. The tables are being identified by their index in the passed
// slice.
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
        for len in 0..header.len() {
            res.entry(header[..len].into())
                .or_insert_with(Vec::new)
                .push(i);
        }
    }
    res
}

struct Table<'de> {
    at: usize,
    end: usize,
    header: InlineVec<Key<'de>>,
    values: Option<TableValues<'de>>,
    array: bool,
}

struct TableValues<'de> {
    values: Vec<TablePair<'de>>,
    span: Option<Span>,
}

#[allow(clippy::derivable_impls)]
impl Default for TableValues<'_> {
    fn default() -> Self {
        Self {
            values: Vec::new(),
            span: None,
        }
    }
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
                        values: Some(TableValues::default()),
                        array,
                    };
                    while let Some(part) = header.next().map_err(|e| self.token_error(e))? {
                        cur_table.header.push(part);
                    }
                    cur_table.end = header.tokens.current();
                }
                Line::KeyValue {
                    key,
                    value,
                    at,
                    end,
                } => {
                    let table_values = cur_table.values.get_or_insert_with(|| TableValues {
                        values: Vec::new(),
                        span: None,
                    });
                    self.add_dotted_key(key, value, table_values)?;
                    match table_values.span {
                        Some(ref mut span) => {
                            span.start = span.start.min(at);
                            span.end = span.end.max(end);
                        }
                        None => {
                            table_values.span = Some(Span::new(at, end));
                        }
                    }
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
        let start = self.tokens.current();
        let key = self.dotted_key()?;
        self.eat_whitespace();
        self.expect(Token::Equals)?;
        self.eat_whitespace();

        let value = self.value()?;
        let end = self.tokens.current();
        self.eat_whitespace();
        if !self.eat_comment()? {
            self.eat_newline_or_eof()?;
        }

        Ok(Line::KeyValue {
            key,
            value,
            at: start,
            end,
        })
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
                        ));
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
    fn inline_table(&mut self) -> Result<(Span, TableValues<'a>), Error> {
        let mut ret = TableValues::default();
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
    ///   `vec![Cow::Borrowed("part"), Cow::Borrowed("one")].`
    /// * `value`: The parsed value.
    /// * `values`: The `Vec` to store the value in.
    fn add_dotted_key(
        &self,
        mut key_parts: Vec<Key<'a>>,
        value: Val<'a>,
        values: &mut TableValues<'a>,
    ) -> Result<(), Error> {
        let key = key_parts.remove(0);
        if key_parts.is_empty() {
            values.values.push((key, value));
            return Ok(());
        }
        match values
            .values
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
            e: E::DottedTable(TableValues::default()),
            start: value.start,
            end: value.end,
        };
        values.values.push((key, table_values));
        let last_i = values.values.len() - 1;
        if let (
            _,
            Val {
                e: E::DottedTable(ref mut v),
                ..
            },
        ) = values.values[last_i]
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
    KeyValue {
        at: usize,
        end: usize,
        key: Vec<Key<'a>>,
        value: Val<'a>,
    },
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

struct Val<'a> {
    e: E<'a>,
    start: usize,
    end: usize,
}

enum E<'a> {
    Integer(i64),
    Float(f64),
    Boolean(bool),
    String(DeStr<'a>),
    Array(Vec<Val<'a>>),
    InlineTable(TableValues<'a>),
    DottedTable(TableValues<'a>),
}

impl E<'_> {
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
