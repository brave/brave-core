// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

//! Parsing data sent with a `<form method="POST">`.
//!
//! In order to parse the body of a request, you can use the `post_input!` macro.
//!
//! ```
//! # #[macro_use] extern crate rouille;
//! use rouille::Request;
//! use rouille::Response;
//!
//! fn handle_request(request: &Request) -> Response {
//!     let input = try_or_400!(post_input!(request, {
//!         field1: u32,
//!         field2: String,
//!     }));
//!
//!     Response::text(format!("the value of field1 is: {}", input.field1))
//! }
//! # fn main() {}
//! ```
//!
//! In this example, the macro will read the body of the request and try to find fields whose
//! names are `field1` and `field2`. If the body was already retrieved earlier, if the content-type
//! is not one of the possible values, or if a field is missing or can't be parsed, then an error
//! is returned. Usually you want to handle this error by returning an error to the client.
//!
//! The macro will define and build a struct whose members are the field names that are passed.
//! The macro then returns a `Result<TheGeneratedStruct, PostError>`.
//!
//! # Data types
//!
//! The types that can be used with this macro are the following:
//!
//! - `String`: The value sent by the client is directly put in the `String`.
//! - `u8`/`i8`/`u16`/`i16`/ `u32`/`i32`/ `u64`/`i64`/`usize`/`isize`/`f32`/`f64`: Rouille will try
//!   to parse the number from the data passed by the client. An error is produced if the client
//!   sent a value that failed to parse or that overflows the capacity of the number.
//! - `Option<T>`: This is equivalent to `T`, but if the field is missing or fails to parse then
//!   the `Option` will contain `None` and no error will be produced.
//! - `bool`: Will be `true` if the field is present at least once and `false` if it is absent.
//!   This is suitable to know whether a `<input type="checkbox" />` is checked or not.
//! - `Vec<T>`: Same as `T`, except that if the client sends multiple fields with that name then
//!   they are merged together. If you don't use a `Vec` then an error is returned in that
//!   situation. If the client provides multiple values and some of them fail to parse, an error
//!   is returned. You can use a `Vec<Option<T>>` if you don't want an error on parse failure.
//!   Empty vecs are possible.
//! - The file-uploads-related types. See below.
//!
//! > **Note**: You may find resources on the web telling you that you must put brackets (`[` `]`)
//! > after the name of inputs of type `<select multiple>` and `<input type="file" multiple>`.
//! > This is only necessary for some programming languages and frameworks, and is not relevant
//! > for rouille. With rouille you just need to use a `Vec` for the data type.
//!
//! You can also use your own types by implementing the
//! [`DecodePostField` trait](trait.DecodePostField.html). See below.
//!
//! # Handling file uploads
//!
//! In order to receive a file sent with a `<form>`, you should use one of the provided structs
//! that represent a file:
//!
//! - [`BufferedFile`](struct.BufferedFile.html), in which case the body of the file will be stored
//!   in memory.
//!
//! Example:
//!
//! ```
//! # #[macro_use] extern crate rouille;
//! use rouille::Request;
//! use rouille::Response;
//! use rouille::input::post::BufferedFile;
//!
//! fn handle_request(request: &Request) -> Response {
//!     let input = try_or_400!(post_input!(request, {
//!         file: BufferedFile,
//!     }));
//!
//!     Response::text("everything ok")
//! }
//! # fn main() {}
//! ```
//!
//! # How it works internally
//!
//! In order for the macro to work, each type of data (like `u32`, `String` or `BufferedFile`) must
//! implement the [`DecodePostField` trait](trait.DecodePostField.html).
//!
//! The template parameter of the trait represents the type of the configuration object that is
//! accepted by the methods. If the user doesn't specify any configuration, the type will be `()`.
//!
//! When rouille's parser finds a field with the correct name it will attempt to call the
//! `from_field` method, and if it find a file with the correct name it will attempt to call the
//! `from_file` method. You should return `PostFieldError::WrongFieldType` if you're
//! expecting a file and `from_field` was called, or vice-versa.

use Request;

use std::borrow::Cow;
use std::error;
use std::fmt;
use std::io::BufRead;
use std::io::Error as IoError;
use std::io::Read;
use std::num;

// Must be made public so that it can be used by the `post_input` macro.
#[doc(hidden)]
pub use url::form_urlencoded;

/// Error that can happen when decoding POST data.
#[derive(Debug)]
pub enum PostError {
    /// The `Content-Type` header of the request indicates that it doesn't contain POST data.
    WrongContentType,

    /// Can't parse the body of the request because it was already extracted.
    BodyAlreadyExtracted,

    /// Could not read the body from the request.
    IoError(IoError),

    /// Failed to parse a string field.
    NotUtf8(String),

    /// There was an error with a particular field.
    Field {
        field: Cow<'static, str>,
        error: PostFieldError,
    },
}

impl From<IoError> for PostError {
    #[inline]
    fn from(err: IoError) -> PostError {
        PostError::IoError(err)
    }
}

impl error::Error for PostError {
    #[inline]
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match *self {
            PostError::IoError(ref e) => Some(e),
            PostError::Field { ref error, .. } => Some(error),
            _ => None,
        }
    }
}

impl fmt::Display for PostError {
    #[inline]
    fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        let description = match *self {
            PostError::BodyAlreadyExtracted => "the body of the request was already extracted",
            PostError::WrongContentType => "the request didn't have a post content type",
            PostError::IoError(_) => {
                "could not read the body from the request, or could not execute the CGI program"
            }
            PostError::NotUtf8(_) => {
                "the content-type encoding is not ASCII or UTF-8, or the body is not valid UTF-8"
            }
            PostError::Field { .. } => "failed to parse a requested field",
        };

        write!(fmt, "{}", description)
    }
}

/// Error returned by the methods of [the `DecodePostField` trait](trait.DecodePostField.html).
#[derive(Debug)]
pub enum PostFieldError {
    /// Could not read the body. Usually happens with files.
    IoError(IoError),

    /// A field is missing from the received data.
    MissingField,

    /// Expected a file but got a field, or vice versa.
    WrongFieldType,

    /// Got multiple values for the same field while only one was expected.
    UnexpectedMultipleValues,

    /// Failed to parse an integer field.
    WrongDataTypeInt(num::ParseIntError),

    /// Failed to parse a floating-point field.
    WrongDataTypeFloat(num::ParseFloatError),
}

impl From<IoError> for PostFieldError {
    #[inline]
    fn from(err: IoError) -> PostFieldError {
        PostFieldError::IoError(err)
    }
}

impl From<num::ParseIntError> for PostFieldError {
    #[inline]
    fn from(err: num::ParseIntError) -> PostFieldError {
        PostFieldError::WrongDataTypeInt(err)
    }
}

impl From<num::ParseFloatError> for PostFieldError {
    #[inline]
    fn from(err: num::ParseFloatError) -> PostFieldError {
        PostFieldError::WrongDataTypeFloat(err)
    }
}

impl error::Error for PostFieldError {
    #[inline]
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        match *self {
            PostFieldError::IoError(ref e) => Some(e),
            PostFieldError::WrongDataTypeInt(ref e) => Some(e),
            PostFieldError::WrongDataTypeFloat(ref e) => Some(e),
            _ => None,
        }
    }
}

impl fmt::Display for PostFieldError {
    #[inline]
    fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        let description = match *self {
            PostFieldError::IoError(_) => {
                "could not read the body from the request, or could not execute the CGI program"
            }
            PostFieldError::MissingField => "the field is missing from the request's client",
            PostFieldError::WrongFieldType => "expected a file but got a field, or vice versa",
            PostFieldError::UnexpectedMultipleValues => {
                "got multiple values for the same field while only one was expected"
            }
            PostFieldError::WrongDataTypeInt(_) => "failed to parse an integer field",
            PostFieldError::WrongDataTypeFloat(_) => "failed to parse a floating-point field",
        };

        write!(fmt, "{}", description)
    }
}

/// Must be implemented on types used with the `post_input!` macro.
///
/// The template parameter represents the type of a configuration object that can be passed by
/// the user when the macro is called. If the user doesn't pass any configuration, the expected
/// type is `()`.
pub trait DecodePostField<Config>: fmt::Debug {
    /// Called when a field with the given name is found in the POST input.
    ///
    /// The value of `content` is what the client sent. This function should attempt to parse it
    /// into `Self` or return an error if it couldn't. If `Self` can't handle a field, then a
    /// `PostFieldError::WrongFieldType` error should be returned.
    fn from_field(config: Config, content: &str) -> Result<Self, PostFieldError>
    where
        Self: Sized;

    /// Called when a file with the given name is found in the POST input.
    ///
    /// The `file` is an object from which the body of the file can be read. The `filename` and
    /// `mime` are also arbitrary values sent directly by the client, so you shouldn't trust them
    /// blindly.
    ///
    /// > **Note**: The `file` object can typically read directly from the socket. But don't worry
    /// > about doing something wrong, as there are protection mechanisms that will prevent you
    /// > from reading too far.
    ///
    /// This method should do something with the file (like storing it somewhere) and return a
    /// `Self` that will allow the user to manipulate the file that was uploaded.
    ///
    /// If `Self` can't handle a file, then a `PostFieldError::WrongFieldType` error should
    /// be returned.
    fn from_file<R>(
        config: Config,
        file: R,
        filename: Option<&str>,
        mime: &str,
    ) -> Result<Self, PostFieldError>
    where
        Self: Sized,
        R: BufRead;

    /// When multiple fields with the same name are found in the client's input, rouille will build
    /// an object for each of them and then merge them with this method.
    ///
    /// The default implementation returns `UnexpectedMultipleValues`.
    fn merge_multiple(self, _existing: Self) -> Result<Self, PostFieldError>
    where
        Self: Sized,
    {
        Err(PostFieldError::UnexpectedMultipleValues)
    }

    /// Called when no field is found in the POST input.
    ///
    /// The default implementation returns `MissingField`.
    #[inline]
    fn not_found(_: Config) -> Result<Self, PostFieldError>
    where
        Self: Sized,
    {
        Err(PostFieldError::MissingField)
    }
}

macro_rules! impl_decode_post_field_decode {
    ($t:ident) => {
        impl DecodePostField<()> for $t {
            fn from_field(_: (), content: &str) -> Result<Self, PostFieldError> {
                Ok(match content.parse() {
                    Ok(v) => v,
                    Err(err) => return Err(err.into()),
                })
            }

            fn from_file<R>(_: (), _: R, _: Option<&str>, _: &str) -> Result<Self, PostFieldError>
            where
                R: BufRead,
            {
                Err(PostFieldError::WrongFieldType)
            }
        }
    };
}

impl_decode_post_field_decode!(u8);
impl_decode_post_field_decode!(i8);
impl_decode_post_field_decode!(u16);
impl_decode_post_field_decode!(i16);
impl_decode_post_field_decode!(u32);
impl_decode_post_field_decode!(i32);
impl_decode_post_field_decode!(u64);
impl_decode_post_field_decode!(i64);
impl_decode_post_field_decode!(usize);
impl_decode_post_field_decode!(isize);
impl_decode_post_field_decode!(f32);
impl_decode_post_field_decode!(f64);

impl DecodePostField<()> for String {
    fn from_field(_: (), content: &str) -> Result<Self, PostFieldError> {
        Ok(content.to_owned())
    }

    fn from_file<R>(_: (), _: R, _: Option<&str>, _: &str) -> Result<Self, PostFieldError>
    where
        R: BufRead,
    {
        Err(PostFieldError::WrongFieldType)
    }
}

impl<T, C> DecodePostField<C> for Option<T>
where
    T: DecodePostField<C>,
{
    fn from_field(config: C, content: &str) -> Result<Self, PostFieldError> {
        match DecodePostField::from_field(config, content) {
            Ok(val) => Ok(Some(val)),
            Err(_) => Ok(None),
        }
    }

    fn from_file<R>(
        config: C,
        file: R,
        filename: Option<&str>,
        mime: &str,
    ) -> Result<Self, PostFieldError>
    where
        R: BufRead,
    {
        match DecodePostField::from_file(config, file, filename, mime) {
            Ok(val) => Ok(Some(val)),
            Err(_) => Ok(None),
        }
    }

    #[inline]
    fn not_found(_: C) -> Result<Self, PostFieldError> {
        Ok(None)
    }
}

impl DecodePostField<()> for bool {
    #[inline]
    fn from_field(_: (), _: &str) -> Result<Self, PostFieldError> {
        Ok(true)
    }

    #[inline]
    fn from_file<R>(_: (), _: R, _: Option<&str>, _: &str) -> Result<Self, PostFieldError>
    where
        R: BufRead,
    {
        Ok(true)
    }

    #[inline]
    fn merge_multiple(self, existing: bool) -> Result<bool, PostFieldError> {
        Ok(self || existing)
    }

    #[inline]
    fn not_found(_: ()) -> Result<Self, PostFieldError> {
        Ok(false)
    }
}

impl<T, C> DecodePostField<C> for Vec<T>
where
    T: DecodePostField<C>,
{
    fn from_field(config: C, content: &str) -> Result<Self, PostFieldError> {
        Ok(vec![DecodePostField::from_field(config, content)?])
    }

    fn from_file<R>(
        config: C,
        file: R,
        filename: Option<&str>,
        mime: &str,
    ) -> Result<Self, PostFieldError>
    where
        R: BufRead,
    {
        Ok(vec![DecodePostField::from_file(
            config, file, filename, mime,
        )?])
    }

    fn merge_multiple(mut self, mut existing: Vec<T>) -> Result<Vec<T>, PostFieldError> {
        self.append(&mut existing);
        Ok(self)
    }

    #[inline]
    fn not_found(_: C) -> Result<Self, PostFieldError> {
        Ok(Vec::new())
    }
}

/// Implementation of the `DecodePostField` that puts the body of the file in memory.
#[derive(Clone)]
pub struct BufferedFile {
    /// The file's data.
    pub data: Vec<u8>,
    /// The MIME type. Remember that this shouldn't be blindly trusted.
    pub mime: String,
    /// The name of the file, if known. Remember that this shouldn't be blindly trusted.
    pub filename: Option<String>,
}

impl fmt::Debug for BufferedFile {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        fmt.debug_struct("BufferedFile")
            .field("data", &format!("<{} bytes>", self.data.len()))
            .field("mime", &self.mime)
            .field("filename", &self.filename)
            .finish()
    }
}

impl DecodePostField<()> for BufferedFile {
    fn from_field(_: (), _: &str) -> Result<Self, PostFieldError> {
        Err(PostFieldError::WrongFieldType)
    }

    fn from_file<R>(
        _: (),
        mut file: R,
        filename: Option<&str>,
        mime: &str,
    ) -> Result<Self, PostFieldError>
    where
        R: BufRead,
    {
        let mut out = Vec::new();
        file.read_to_end(&mut out)?;

        Ok(BufferedFile {
            data: out,
            mime: mime.to_owned(),
            filename: filename.map(|n| n.to_owned()),
        })
    }
}

/// Parse input from HTML forms. See [the `post` module](input/post/index.html) for general
/// documentation.
#[macro_export]
macro_rules! post_input {
    ($request:expr, {$($field:ident: $ty:ty $({$config:expr})*),*$(,)*}) => ({
        use std::io::Read;
        use std::result::Result;
        use $crate::Request;
        use $crate::input::post::DecodePostField;
        use $crate::input::post::PostFieldError;
        use $crate::input::post::PostError;
        use $crate::input::post::form_urlencoded;
        use $crate::input::multipart;

        #[derive(Debug)]
        struct PostInput {
            $(
                $field: $ty,
            )*
        }

        fn merge<C, T: DecodePostField<C>>(existing: &mut Option<T>, new: T)
                                           -> Result<(), PostFieldError>
        {
            match existing {
                a @ &mut Some(_) => {
                    let extracted = a.take().unwrap();
                    let merged = extracted.merge_multiple(new)?;
                    *a = Some(merged);
                },
                a @ &mut None => *a = Some(new),
            };

            Ok(())
        }

        fn go(request: &Request) -> Result<PostInput, PostError> {
            $(
                let mut $field: Option<$ty> = None;
            )*

            // TODO: handle if the same field is specified multiple times

            if request.header("Content-Type").map(|ct| ct.starts_with("application/x-www-form-urlencoded")).unwrap_or(false) {
                let body = {
                    // TODO: DDoSable server if body is too large?
                    let mut out = Vec::new();       // TODO: with_capacity()?
                    if let Some(mut b) = request.data() {
                        b.read_to_end(&mut out)?;
                    } else {
                        return Err(PostError::BodyAlreadyExtracted);
                    }
                    out
                };

                for (field, value) in form_urlencoded::parse(&body) {
                    $(
                        if field == stringify!($field) {
                            let config = ();
                            $(
                                let config = $config;
                            )*

                            let decoded = match DecodePostField::from_field(config, &value) {
                                Ok(d) => d,
                                Err(err) => return Err(PostError::Field {
                                    field: stringify!($field).into(),
                                    error: err,
                                }),
                            };

                            match merge(&mut $field, decoded) {
                                Ok(d) => d,
                                Err(err) => return Err(PostError::Field {
                                    field: stringify!($field).into(),
                                    error: err,
                                }),
                            };
                            continue;
                        }
                    )*
                }

            } else {
                let mut multipart = match multipart::get_multipart_input(request) {
                    Ok(m) => m,
                    Err(multipart::MultipartError::WrongContentType) => {
                        return Err(PostError::WrongContentType);
                    },
                    Err(multipart::MultipartError::BodyAlreadyExtracted) => {
                        return Err(PostError::BodyAlreadyExtracted);
                    },
                };

                while let Some(mut multipart_entry) = multipart.next() {
                    $(
                        if multipart_entry.headers.name.as_ref() == stringify!($field) {
                            let config = ();
                            $(
                                let config = $config;
                            )*

                            if multipart_entry.is_text() {
                                let mut text = String::new();
                                multipart_entry.data.read_to_string(&mut text)?;
                                let decoded = match DecodePostField::from_field(config, &text) {
                                    Ok(d) => d,
                                    Err(err) => return Err(PostError::Field {
                                        field: stringify!($field).into(),
                                        error: err,
                                    }),
                                };
                                match merge(&mut $field, decoded) {
                                    Ok(d) => d,
                                    Err(err) => return Err(PostError::Field {
                                        field: stringify!($field).into(),
                                        error: err,
                                    }),
                                };
                            } else {
                                let name = multipart_entry.headers.filename.as_ref().map(|n| n.to_owned());
                                let name = name.as_ref().map(|n| &n[..]);
                                let mime = multipart_entry.headers.content_type
                                    .map(|m| m.to_string())
                                    .unwrap_or_else(String::new);
                                let decoded = match DecodePostField::from_file(config, multipart_entry.data, name, &mime) {
                                    Ok(d) => d,
                                    Err(err) => return Err(PostError::Field {
                                        field: stringify!($field).into(),
                                        error: err,
                                    }),
                                };
                                match merge(&mut $field, decoded) {
                                    Ok(d) => d,
                                    Err(err) => return Err(PostError::Field {
                                        field: stringify!($field).into(),
                                        error: err,
                                    }),
                                };
                            }
                            continue;
                        }
                    )*
                }
            }

            Ok(PostInput {
                $(
                    $field: match $field {
                        Some(v) => v,
                        None => {
                            let config = ();
                            $(
                                let config = $config;
                            )*

                            match DecodePostField::not_found(config) {
                                Ok(d) => d,
                                Err(err) => return Err(PostError::Field {
                                    field: stringify!($field).into(),
                                    error: err,
                                }),
                            }
                        }
                    },
                )*
            })
        }

        go($request)
    });
}

/// Attempts to decode the `POST` data received by the request.
///
/// If successful, returns a list of fields and values.
///
/// Returns an error if the request's content-type is not related to POST data.
// TODO: what to do with this function?
pub fn raw_urlencoded_post_input(request: &Request) -> Result<Vec<(String, String)>, PostError> {
    if request
        .header("Content-Type")
        .map(|ct| !ct.starts_with("application/x-www-form-urlencoded"))
        .unwrap_or(true)
    {
        return Err(PostError::WrongContentType);
    }

    let body = {
        // TODO: DDoSable server if body is too large?
        let mut out = Vec::new(); // TODO: with_capacity()?
        if let Some(mut b) = request.data() {
            b.read_to_end(&mut out)?;
        } else {
            return Err(PostError::BodyAlreadyExtracted);
        }
        out
    };

    Ok(form_urlencoded::parse(&body).into_owned().collect()) // TODO: suboptimal
}

#[cfg(test)]
mod tests {
    use input::post::PostError;
    use input::post::PostFieldError;
    use Request;

    #[test]
    fn basic_int() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12".to_vec(),
        );

        let input = post_input!(&request, { field: u32 }).unwrap();

        assert_eq!(input.field, 12);
    }

    #[test]
    fn basic_float() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12.8".to_vec(),
        );

        let input = post_input!(&request, { field: f32 }).unwrap();

        assert_eq!(input.field, 12.8);
    }

    #[test]
    fn basic_string() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=value".to_vec(),
        );

        let input = post_input!(&request, { field: String }).unwrap();

        assert_eq!(input.field, "value");
    }

    #[test]
    fn basic_option_string() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=value".to_vec(),
        );

        let input = post_input!(&request, {
            field: Option<String>
        })
        .unwrap();

        assert_eq!(input.field.unwrap(), "value");
    }

    #[test]
    fn basic_bool() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=value".to_vec(),
        );

        let input = post_input!(&request, { field: bool }).unwrap();

        assert_eq!(input.field, true);
    }

    #[test]
    fn weird_stuff() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"&=&aa&b=&c=c=c&field=value&".to_vec(),
        );

        let input = post_input!(&request, { field: String }).unwrap();

        assert_eq!(input.field, "value");
    }

    #[test]
    fn wrong_content_type() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                ("Content-Type".to_owned(), "wrong".to_owned()),
            ],
            b"field=value".to_vec(),
        );

        let input = post_input!(&request, { field: String });

        match input {
            Err(PostError::WrongContentType) => (),
            _ => panic!(),
        }
    }

    #[test]
    fn too_many_fields() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12&field2=58".to_vec(),
        );

        let input = post_input!(&request, { field: u32 }).unwrap();

        assert_eq!(input.field, 12);
    }

    #[test]
    fn multiple_values() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12&field=58".to_vec(),
        );

        let input = post_input!(&request, { field: u32 });

        match input {
            Err(PostError::Field {
                ref field,
                error: PostFieldError::UnexpectedMultipleValues,
            }) if field == "field" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn multiple_values_bool() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12&field=58".to_vec(),
        );

        let input = post_input!(&request, { field: bool }).unwrap();

        assert_eq!(input.field, true);
    }

    #[test]
    fn multiple_values_vec() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12&field=58".to_vec(),
        );

        let input = post_input!(&request, {
            field: Vec<u32>
        })
        .unwrap();

        assert_eq!(input.field, &[12, 58]);
    }

    #[test]
    fn multiple_values_vec_parse_failure() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12&field=800".to_vec(),
        );

        let input = post_input!(&request, {
            field: Vec<u8>
        });

        match input {
            Err(PostError::Field {
                ref field,
                error: PostFieldError::WrongDataTypeInt(_),
            }) if field == "field" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn multiple_values_vec_option_parse_failure() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12&field=800".to_vec(),
        );

        let input = post_input!(&request, {
            field: Vec<Option<u8>>
        })
        .unwrap();

        assert_eq!(input.field, &[Some(12), None]);
    }

    #[test]
    fn missing_field() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"wrong_field=value".to_vec(),
        );

        let input = post_input!(&request, { field: String });

        match input {
            Err(PostError::Field {
                ref field,
                error: PostFieldError::MissingField,
            }) if field == "field" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn missing_field_option() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"wrong=value".to_vec(),
        );

        let input = post_input!(&request, {
            field: Option<String>
        })
        .unwrap();

        assert_eq!(input.field, None);
    }

    #[test]
    fn missing_field_bool() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"wrong=value".to_vec(),
        );

        let input = post_input!(&request, { field: bool }).unwrap();

        assert_eq!(input.field, false);
    }

    #[test]
    fn missing_field_vec() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"wrong=value".to_vec(),
        );

        let input = post_input!(&request, {
            field: Vec<String>
        })
        .unwrap();

        assert!(input.field.is_empty());
    }

    #[test]
    fn num_parse_error() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12foo".to_vec(),
        );

        let input = post_input!(&request, { field: u32 });

        match input {
            Err(PostError::Field {
                ref field,
                error: PostFieldError::WrongDataTypeInt(_),
            }) if field == "field" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn num_parse_error_option() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=12foo".to_vec(),
        );

        let input = post_input!(&request, {
            field: Option<u32>
        })
        .unwrap();

        assert_eq!(input.field, None);
    }

    #[test]
    fn num_overflow() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=800".to_vec(),
        );

        let input = post_input!(&request, { field: u8 });

        match input {
            Err(PostError::Field {
                ref field,
                error: PostFieldError::WrongDataTypeInt(_),
            }) if field == "field" => (),
            _ => panic!(),
        }
    }

    #[test]
    fn body_extracted() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=800".to_vec(),
        );

        let _ = request.data();

        let input = post_input!(&request, { field: u8 });

        match input {
            Err(PostError::BodyAlreadyExtracted) => (),
            _ => panic!(),
        }
    }

    #[test]
    #[ignore] // FIXME:
    fn not_utf8() {
        let request = Request::fake_http(
            "GET",
            "/",
            vec![
                ("Host".to_owned(), "localhost".to_owned()),
                (
                    "Content-Type".to_owned(),
                    "application/x-www-form-urlencoded".to_owned(),
                ),
            ],
            b"field=\xc3\x28".to_vec(),
        );

        let input = post_input!(&request, { field: String });

        match input {
            Err(PostError::NotUtf8(_)) => (),
            v => panic!("{:?}", v),
        }
    }

    // TODO: add tests for multipart/form-data
}
