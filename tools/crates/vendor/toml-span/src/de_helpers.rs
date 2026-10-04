//! Provides helpers for deserializing [`Value`]/[`ValueInner`] into Rust types

use crate::{
    DeserError, Deserialize, Error, ErrorKind, Span,
    span::Spanned,
    value::{self, Table, Value, ValueInner},
};
use std::{fmt::Display, str::FromStr};

/// Helper for construction an [`ErrorKind::Wanted`]
#[inline]
pub fn expected(expected: &'static str, found: ValueInner<'_>, span: Span) -> Error {
    Error {
        kind: ErrorKind::Wanted {
            expected,
            found: found.type_str(),
        },
        span,
        line_info: None,
    }
}

/// Attempts to acquire a [`ValueInner::String`] and parse it, returning an error
/// if the value is not a string, or the parse implementation fails
#[inline]
pub fn parse<T, E>(value: &mut Value<'_>) -> Result<T, Error>
where
    T: FromStr<Err = E>,
    E: Display,
{
    let s = value.take_string(None)?;
    match s.parse() {
        Ok(v) => Ok(v),
        Err(err) => Err(Error {
            kind: ErrorKind::Custom(format!("failed to parse string: {err}").into()),
            span: value.span,
            line_info: None,
        }),
    }
}

/// A helper for dealing with [`ValueInner::Table`]
pub struct TableHelper<'de> {
    /// The table the helper is operating upon
    pub table: Table<'de>,
    /// The errors accumulated while deserializing
    pub errors: Vec<Error>,
    /// The list of keys that have been requested by the user, this is used to
    /// show a list of keys that _could_ be used in the case the finalize method
    /// fails due to keys still being present in the map
    expected: Vec<&'static str>,
    /// The span for the table location
    span: Span,
}

impl<'de> From<(Table<'de>, Span)> for TableHelper<'de> {
    fn from((table, span): (Table<'de>, Span)) -> Self {
        Self {
            table,
            span,
            expected: Vec::new(),
            errors: Vec::new(),
        }
    }
}

impl<'de> TableHelper<'de> {
    /// Creates a helper for the value, failing if it is not a table
    pub fn new(value: &mut Value<'de>) -> Result<Self, DeserError> {
        let table = match value.take() {
            ValueInner::Table(table) => table,
            other => return Err(expected("a table", other, value.span).into()),
        };

        Ok(Self {
            errors: Vec::new(),
            table,
            expected: Vec::new(),
            span: value.span,
        })
    }

    /// Returns true if the table contains the specified key
    #[inline]
    pub fn contains(&self, name: &str) -> bool {
        self.table.contains_key(name)
    }

    /// Takes the specified key and its value if it exists
    #[inline]
    pub fn take(&mut self, name: &'static str) -> Option<(value::Key<'de>, Value<'de>)> {
        self.expected.push(name);
        self.table.remove_entry(name)
    }

    /// Attempts to deserialize the specified key
    ///
    /// Errors that occur when calling this method are automatically added to
    /// the set of errors that are reported from [`Self::finalize`], so not early
    /// returning if this method fails will still report the error by default
    ///
    /// # Errors
    /// - The key does not exist
    /// - The [`Deserialize`] implementation for the type returns an error
    #[inline]
    pub fn required<T: Deserialize<'de>>(&mut self, name: &'static str) -> Result<T, Error> {
        Ok(self.required_s(name)?.value)
    }

    /// The same as [`Self::required`], except it returns a [`Spanned`]
    pub fn required_s<T: Deserialize<'de>>(
        &mut self,
        name: &'static str,
    ) -> Result<Spanned<T>, Error> {
        self.expected.push(name);

        let Some(mut val) = self.table.remove(name) else {
            let missing = Error {
                kind: ErrorKind::MissingField(name),
                span: self.span,
                line_info: None,
            };
            self.errors.push(missing.clone());
            return Err(missing);
        };

        Spanned::<T>::deserialize(&mut val).map_err(|mut errs| {
            let err = errs.errors.last().unwrap().clone();
            self.errors.append(&mut errs.errors);
            err
        })
    }

    /// Attempts to deserialize the specified key, if it exists
    ///
    /// Note that if the key exists but deserialization fails, an error will be
    /// appended and if [`Self::finalize`] is called it will return that error
    /// along with any others that occurred
    #[inline]
    pub fn optional<T: Deserialize<'de>>(&mut self, name: &'static str) -> Option<T> {
        self.optional_s(name).map(|v| v.value)
    }

    /// The same as [`Self::optional`], except it returns a [`Spanned`]
    pub fn optional_s<T: Deserialize<'de>>(&mut self, name: &'static str) -> Option<Spanned<T>> {
        self.expected.push(name);

        let mut val = self.table.remove(name)?;

        match Spanned::<T>::deserialize(&mut val) {
            Ok(v) => Some(v),
            Err(mut err) => {
                self.errors.append(&mut err.errors);
                None
            }
        }
    }

    /// Called when you are finished with this [`TableHelper`]
    ///
    /// If errors have been accumulated when using this [`TableHelper`], this will
    /// return an error with all of those errors.
    ///
    /// Additionally, if [`Option::None`] is passed, any keys that still exist
    /// in the table will be added to an [`ErrorKind::UnexpectedKeys`] error,
    /// which can be considered equivalent to [`#[serde(deny_unknown_fields)]`](https://serde.rs/container-attrs.html#deny_unknown_fields)
    ///
    /// If you want simulate [`#[serde(flatten)]`](https://serde.rs/field-attrs.html#flatten)
    /// you can instead put that table back in its original value during this step
    pub fn finalize(mut self, original: Option<&mut Value<'de>>) -> Result<(), DeserError> {
        if let Some(original) = original {
            original.set(ValueInner::Table(self.table));
        } else if !self.table.is_empty() {
            let keys = self
                .table
                .into_keys()
                .map(|key| (key.name.into(), key.span))
                .collect();

            self.errors.push(
                (
                    ErrorKind::UnexpectedKeys {
                        keys,
                        expected: self.expected.into_iter().map(String::from).collect(),
                    },
                    self.span,
                )
                    .into(),
            );
        }

        if self.errors.is_empty() {
            Ok(())
        } else {
            Err(DeserError {
                errors: self.errors,
            })
        }
    }
}

impl<'de> Deserialize<'de> for String {
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        value
            .take_string(None)
            .map(|s| s.into())
            .map_err(DeserError::from)
    }
}

impl<'de> Deserialize<'de> for std::borrow::Cow<'de, str> {
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        value.take_string(None).map_err(DeserError::from)
    }
}

impl<'de> Deserialize<'de> for bool {
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        match value.take() {
            ValueInner::Boolean(b) => Ok(b),
            other => Err(expected("a bool", other, value.span).into()),
        }
    }
}

macro_rules! integer {
    ($num:ty) => {
        impl<'de> Deserialize<'de> for $num {
            fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
                match value.take() {
                    ValueInner::Integer(i) => {
                        let i = i.try_into().map_err(|_| {
                            DeserError::from(Error {
                                kind: ErrorKind::OutOfRange(stringify!($num)),
                                span: value.span,
                                line_info: None,
                            })
                        })?;

                        Ok(i)
                    }
                    other => Err(expected(stringify!($num), other, value.span).into()),
                }
            }
        }
    };
}

integer!(u8);
integer!(u16);
integer!(u32);
integer!(u64);
integer!(i8);
integer!(i16);
integer!(i32);
integer!(i64);
integer!(usize);
integer!(isize);

impl<'de> Deserialize<'de> for f32 {
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        match value.take() {
            ValueInner::Float(f) => Ok(f as f32),
            other => Err(expected("a float", other, value.span).into()),
        }
    }
}

impl<'de> Deserialize<'de> for f64 {
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        match value.take() {
            ValueInner::Float(f) => Ok(f),
            other => Err(expected("a float", other, value.span).into()),
        }
    }
}

impl<'de, T> Deserialize<'de> for Vec<T>
where
    T: Deserialize<'de>,
{
    fn deserialize(value: &mut value::Value<'de>) -> Result<Self, DeserError> {
        match value.take() {
            ValueInner::Array(arr) => {
                let mut errors = Vec::new();
                let mut s = Vec::new();
                for mut v in arr {
                    match T::deserialize(&mut v) {
                        Ok(v) => s.push(v),
                        Err(mut err) => errors.append(&mut err.errors),
                    }
                }

                if errors.is_empty() {
                    Ok(s)
                } else {
                    Err(DeserError { errors })
                }
            }
            other => Err(expected("an array", other, value.span).into()),
        }
    }
}
