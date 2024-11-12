/// Initialize a struct or tuple out of a sequences of parsers
///
/// Unlike normal struct initialization syntax:
/// - `_` fields can exist to run a parser but ignore the result
/// - Parse results for a field can later be referenced using the field name
///
/// Unlike normal tuple initialization syntax:
/// - Struct-style initialization (`{ 0: _, 1: _}`) is not supported
/// - `_: <parser>` fields can exist to run a parser but ignore the result
///
///# Example
///
/// ```
/// # use winnow::prelude::*;
/// # use winnow::ascii::{alphanumeric1, dec_uint, space0};
/// # use winnow::combinator::delimited;
/// # use winnow::combinator::empty;
/// # use winnow::error::ContextError;
/// use winnow::combinator::seq;
///
/// #[derive(Default, Debug, PartialEq)]
/// struct Field {
///     namespace: u32,
///     name: Vec<u8>,
///     value: Vec<u8>,
///     point: (u32, u32),
///     metadata: Vec<u8>,
/// }
///
/// // Parse into structs / tuple-structs
/// fn field(input: &mut &[u8]) -> PResult<Field> {
///     seq!{Field {
///         namespace: empty.value(5),
///         name: alphanumeric1.map(|s: &[u8]| s.to_owned()),
///         // `_` fields are ignored when building the struct
///         _: (space0, b':', space0),
///         value: alphanumeric1.map(|s: &[u8]| s.to_owned()),
///         _: (space0, b':', space0),
///         point: point,
///         // default initialization also works
///         ..Default::default()
///     }}.parse_next(input)
/// }
///
/// // Or parse into tuples
/// fn point(input: &mut &[u8]) -> PResult<(u32, u32)> {
///     let num = dec_uint::<_, u32, ContextError>;
///     seq!(num, _: (space0, b',', space0), num).parse_next(input)
/// }
///
/// assert_eq!(
///     field.parse_peek(&b"test: data: 123 , 4"[..]),
///     Ok((
///         &b""[..],
///         Field {
///             namespace: 5,
///             name: b"test"[..].to_owned(),
///             value: b"data"[..].to_owned(),
///             point: (123, 4),
///             metadata: Default::default(),
///         },
///     )),
/// );
/// ```
#[macro_export]
#[doc(alias = "tuple")]
#[doc(alias = "preceded")]
#[doc(alias = "terminated")]
#[doc(alias = "delimited")]
#[doc(alias = "pair")]
#[doc(alias = "separated_pair")]
#[doc(alias = "struct_parser")]
#[doc(hidden)] // forced to be visible in intended location
macro_rules! seq {
    ($($name: ident)::* { $($fields: tt)* }) => {
        $crate::combinator::trace(stringify!($($name)::*), move |input: &mut _| {
            $crate::seq_parse_struct_fields!(input; $($fields)*);
            #[allow(clippy::redundant_field_names)]
            Ok($crate::seq_init_struct_fields!( ($($fields)*); $($name)::*;))
        })
    };
    ($($name: ident)::* ( $($elements: tt)* )) => {
        $crate::combinator::trace(stringify!($($name)::*), move |input: &mut _| {
            $crate::seq_parse_tuple_fields!( ($($elements)*) ; ).map(|t| {
                $crate::seq_init_tuple_fields!(
                    ($($elements)*);
                    (t.0, t.1, t.2, t.3, t.4, t.5, t.6, t.7, t.8, t.9, t.10, t.11, t.12, t.13, t.14, t.15, t.16, t.17, t.18, t.19, t.20);
                    $($name)::*;
                )
            }).parse_next(input)
        })
    };
    (( $($elements: tt)* )) => {
        $crate::combinator::trace("tuple", move |input: &mut _| {
            use $crate::Parser;
            $crate::seq_parse_tuple_fields!( ($($elements)*) ; ).map(|t| {
                $crate::seq_init_tuple_fields!(
                    ($($elements)*);
                    (t.0, t.1, t.2, t.3, t.4, t.5, t.6, t.7, t.8, t.9, t.10, t.11, t.12, t.13, t.14, t.15, t.16, t.17, t.18, t.19, t.20);
                    ;
                )
            }).parse_next(input)
        })
    };
    ($($elements: tt)*) => {
        $crate::seq!(($($elements)*))
    };
}

#[macro_export]
#[doc(hidden)]
macro_rules! seq_parse_struct_fields {
    (
        $input: ident;
        _ : $head_parser: expr, $($fields: tt)*
    ) => {
        let _ = $crate::Parser::parse_next(&mut $head_parser, $input)?;
        $crate::seq_parse_struct_fields!($input; $($fields)*)
    };
    (
        $input: ident;
        _ : $head_parser: expr
    ) => {
        let _ = $crate::Parser::parse_next(&mut $head_parser, $input)?;
    };
    (
        $input: ident;
        $head_field: ident : $head_parser: expr, $($fields: tt)*
    ) => {
        let $head_field = $crate::Parser::parse_next(&mut $head_parser, $input)?;
        $crate::seq_parse_struct_fields!($input; $($fields)*)
    };
    (
        $input: ident;
        $head_field: ident : $head_parser: expr
    ) => {
        let $head_field = $crate::Parser::parse_next(&mut $head_parser, $input)?;
    };
    (
        $input: expr;
        .. $update: expr
    ) => {};
    (
        $input: expr;
        $(,)?
    ) => {};
}

#[macro_export]
#[doc(hidden)]
macro_rules! seq_parse_tuple_fields {
    (
        (_ : $head_parser: expr, $($fields: tt)* );
        $($sequenced: tt)*
    ) => {
        $crate::seq_parse_tuple_fields!( ( $($fields)* ) ; $($sequenced)* $head_parser.void(), )
    };
    (
        (_ : $head_parser: expr);
        $($sequenced: tt)*
    ) => {
        $crate::seq_parse_tuple_fields!((); $($sequenced)* $head_parser.void(), )
    };
    (
        ($head_parser: expr, $($fields: tt)*);
        $($sequenced: tt)*
    ) => {
        $crate::seq_parse_tuple_fields!( ( $($fields)* ) ; $($sequenced)* $head_parser, )
    };
    (
        ($head_parser: expr);
        $($sequenced: tt)*
    )=> {
        $crate::seq_parse_tuple_fields!((); $($sequenced)* $head_parser, )
    };
    (
        ();
        $($sequenced: tt)*
    ) => {
        ($($sequenced)*)
    };
}

#[macro_export]
#[doc(hidden)]
macro_rules! seq_init_struct_fields {
    (
        (_ : $head_parser: expr, $($fields: tt)*);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $crate::seq_init_struct_fields!( ( $($fields)* ) ; $($name)::* ; $($inits)* )
    };
    (
        (_ : $head_parser: expr);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $crate::seq_init_struct_fields!( (); $($name)::* ; $($inits)* )
    };
    (
        ($head_field: ident : $head_parser: expr, $($fields: tt)*);
        $($name: ident)::*;
        $($inits: tt)*
    ) =>
    {
        $crate::seq_init_struct_fields!( ( $($fields)* ) ; $($name)::* ; $($inits)* $head_field: $head_field, )
    };
    (
        ($head_field: ident : $head_parser: expr);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $crate::seq_init_struct_fields!( (); $($name)::* ; $($inits)* $head_field: $head_field,)
    };
    (
        (.. $update: expr);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $($name)::* { $($inits)* ..$update }
    };
    (
        ($(,)?);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $($name)::* { $($inits)* }
    };
}

#[macro_export]
#[doc(hidden)]
macro_rules! seq_init_tuple_fields {
    (
        (_ : $head_parser: expr, $($fields: tt)*);
        ($head_arg: expr, $($args: expr),*);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $crate::seq_init_tuple_fields!( ( $($fields)* ); ( $($args),* ) ; $($name)::* ; $($inits)* )
    };
    (
        (_ : $head_parser: expr);
        ($head_arg: expr, $($args: expr),*);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $crate::seq_init_tuple_fields!((); ( $($args),* ); $($name)::* ; $($inits)*)
    };
    (
        ($head_parser: expr, $($fields: tt)*);
        ($head_arg: expr, $($args: expr),*);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $crate::seq_init_tuple_fields!( ( $($fields)* ) ; ( $($args),* ) ; $($name)::* ; $($inits)* $head_arg, )
    };
    (
        ($head_parser: expr);
        ($head_arg: expr, $($args: expr),*);
        $($name: ident)::*;
        $($inits: tt)*
    ) => {
        $crate::seq_init_tuple_fields!((); ( $($args),* ); $($name)::* ; $($inits)* $head_arg)
    };
    (
        ();
        ($($args: expr),*);
        $($name: ident)::*;
        $($inits: expr),* $(,)?
    ) => {
        $($name)::*( $($inits,)* )
    };
}
