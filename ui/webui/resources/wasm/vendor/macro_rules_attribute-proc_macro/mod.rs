//! Do not use this crate directly. Instead, use [`::macro_rules_attribute`](
//! https://docs.rs/macro_rules_attribute)

use {
    ::core::{
        ops::Not as _,
        iter::FromIterator as _,
    },
    ::proc_macro::{*,
        TokenTree as TT,
    },
};

// See the re-export at the frontend (`src/lib.rs`) for the documentation.
#[proc_macro_attribute] pub
fn macro_rules_attribute (
    attrs: TokenStream,
    input: TokenStream,
) -> TokenStream
{
    let ret = macro_rules_attribute_impl(&attrs.vec(), input);
    #[cfg(feature = "verbose-expansions")]
    eprintln!("{}", ret);
    ret
}

fn macro_rules_attribute_impl (
    attrs: &'_ [TokenTree],
    input: TokenStream
) -> TokenStream
{
    let mut ret: TokenStream;
    // check that `attrs` is indeed of the form `$macro_name:path !`
    match is_path_bang_terminated(&attrs) {
        | Ok(PathIsBangTerminated(trailing_bang)) => {
            ret = attrs.iter().cloned().collect();
            if trailing_bang {
                /* OK */
            } else {
                // tolerate it
                ret.extend([TT::Punct(Punct::new('!', Spacing::Alone))]);
            }
        },
        | Err(()) => return parse_path_error(attrs),
    }
    ret.extend([TT::Group(Group::new(
        Delimiter::Brace,
        // FIXME: directly using `input` makes the token stream be seen
        // as a single token tree by the declarative macro !??
        input.into_iter().collect(),
    ))]);
    ret
}

// See the re-export at the frontend (`src/lib.rs`) for the documentation.
#[proc_macro_attribute] pub
fn macro_rules_derive (
    attrs: TokenStream,
    input: TokenStream,
) -> TokenStream
{
    let mut ret = TokenStream::new();
    ret.extend(
        attrs
            .vec()
            // we use `split_inclusive()` + strip approach to support trailing
            // comma and yet detect empty input: `derive(Something,,ShouldFail)`
            .split_inclusive(is_punct(','))
            .map(|attr| match attr {
                | [hd @ .., p] if is_punct(',')(p) => hd,
                | _ => attr,
            })
            .flat_map(|attr| macro_rules_attribute_impl(attr, input.clone()))
    );
    ret.extend(real_derive(ts!(::macro_rules_attribute::Custom)));
    ret.extend(input);
    #[cfg(feature = "verbose-expansions")]
    eprintln!("{}", ret);
    ret
}

// See the re-export at the frontend (`src/lib.rs`) for the documentation.
#[proc_macro_attribute] pub
fn derive (
    attrs: TokenStream,
    input: TokenStream,
) -> TokenStream
{
    let attrs = attrs.vec();

    // any `path::to::Macro!` in the derive list?
    if attrs.iter().any(is_punct('!')).not() {
        // This is a good old derive.
        let mut ret = real_derive(attrs.into_iter().collect());
        ret.extend(input);
        #[cfg(feature = "verbose-expansions")]
        eprintln!("{}", ret);
        return ret;
    }

    // Note: emitting the input as-is would break derives that use helper
    // attributes.
    //
    // ```rust
    // let mut ret = input;
    // ret.extend(nested_derive! { #[derive(#real_derives)]})…
    // ```
    //
    // So we have to do it otherwise: we do not emit `input` directly,
    // but rather, with the real derives directly attached to it
    // (no `nested_derive!` invocation at all, here).
    // So it will be the job of each real derive's to strip its inert derive
    // helpers, which is something only a built-in derive can do, hence dodging
    // the issue :)
    let each_attr = || {
        attrs
            // we use `split_inclusive()` + strip approach to support trailing
            // comma and yet detect empty input: `derive(Something,,ShouldFail)`
            .split_inclusive(is_punct(','))
            .map(|attr| match attr {
                | [hd @ .., p] if is_punct(',')(p) => hd,
                | _ => attr,
            })
    };
    let ref each_is_path_bang_terminated =
        each_attr()
            .map(is_path_bang_terminated)
            .vec()
    ;
    for (attr, parse_bang) in each_attr().zip(each_is_path_bang_terminated) {
        if let Err(()) = parse_bang {
            return parse_path_error(attr);
        }
    }
    let attrs_banged = |banged| {
        each_attr()
            .zip(each_is_path_bang_terminated)
            .filter(move |(_, parse_bang)| parse_bang.unwrap().0 == banged)
            .map(|(attr, _)| attr)
    };
    let mut ret = TokenStream::new();
    attrs_banged(true).for_each(|attr| {
        ret.extend(macro_rules_attribute_impl(attr, input.clone()))
    });
    ret.extend(real_derive(
        attrs_banged(false)
            .flat_map(|attr| attr.iter().cloned().chain(ts!(,)))
            .chain(ts!(::macro_rules_attribute::Custom,))
            .collect()
        ,
    ));
    ret.extend(input);

    #[cfg(feature = "verbose-expansions")]
    eprintln!("{}", ret);
    ret
}

#[proc_macro_derive(Custom, attributes(custom, derive_args))] pub
fn custom(_:TokenStream) -> TokenStream {
    TokenStream::new()
}

fn real_derive (
    derives: TokenStream,
) -> TokenStream
{
    // `#[::core::prelude::v1::derive( #derives )]`
    TokenStream::from_iter([
        TT::Punct(Punct::new('#', Spacing::Alone)),
        TT::Group(Group::new(
            Delimiter::Bracket,
            {
                let mut ts: TokenStream = ts!(
                    ::core::prelude::v1::derive
                );
                ts.extend([TT::Group(Group::new(
                    Delimiter::Parenthesis,
                    derives,
                ))]);
                ts
            },
        ))
    ])
}


#[::core::prelude::v1::derive(Clone, Copy)]
struct PathIsBangTerminated(bool);

fn parse_path_error (
    incorrect_input: &[TokenTree],
) -> TokenStream
{
    let mut spans = incorrect_input.iter().map(|tt| tt.span());
    let mut ts = ts!(
        ::core::compile_error! {
            "\
                expected a parameter of the form `path::to::macro_name !` \
                or `path::to::macro_name`.\
            "
        }
    ).vec();
    let fst_span = spans.next().unwrap_or_else(Span::call_site);
    let lst_span = spans.fold(fst_span, |_, cur| cur);
    ts.iter_mut().for_each(|tt| tt.set_span(fst_span));
    ts.last_mut().unwrap().set_span(lst_span);
    ts.into_iter().collect()
}

/// `Ok(… true)` => `some::path!`
/// `Ok(… false)` => `some::path`
/// `Err` => not a (simple) path.
fn is_path_bang_terminated (
    tts: &'_ [TokenTree],
) -> Result<PathIsBangTerminated, ()>
{
    let mut tts = tts.iter().peekable();

    macro_rules! parse_optional_semicolons {() => (
        match tts.peek() {
            | Some(TT::Punct(p)) => {
                let _ = tts.next();
                if p.as_char() == ':' && p.spacing() == Spacing::Joint {
                    match tts.next() {
                        | Some(TT::Punct(p))
                            if p.as_char() == ':'
                            && p.spacing() == Spacing::Alone
                        => {
                            Some(())
                        },
                        | _ => return Err(()),
                    }
                } else {
                    return Err(());
                }
            },
            | _ => None,
        }
    )}

    macro_rules! parse_trailing_comma {() => (
        if tts.peek().copied().map_or(false, is_punct(',')) {
            let _ = tts.next();
            if tts.next().is_some() {
                return Err(());
            }
        }
    )}

    parse_optional_semicolons!();
    // Loop invariant: we start at the identifier part of a path segment.
    loop {
        match tts.next() {
            | Some(TT::Ident(_)) => {},
            | _ => return Err(()),
        }
        // After an identifier, either nothing remains
        // (but for an optional trailing comma)
        parse_trailing_comma!();
        if tts.peek().is_none() {
            return Ok(PathIsBangTerminated(false));
        }
        // or remains a punctuation: either a trailing `!`…
        if tts.peek().copied().map_or(false, is_punct('!')) {
            let _ = tts.next();
            // Now nothing remains (but for an optional trailing comma).
            parse_trailing_comma!();
            return if tts.next().is_none() {
                Ok(PathIsBangTerminated(true))
            } else {
                Err(())
            };
        }
        // …or the `::` separator for yet another segment.
        if parse_optional_semicolons!().is_none() {
            return Err(());
        }
    }
}

fn is_punct (c: char)
  -> impl 'static + Fn(&'_ TokenTree) -> bool
{
    move |tt| matches!(tt, TT::Punct(p) if p.as_char() == c)
}

macro_rules! ts {( $($tt:tt)* ) => (
    ::core::stringify! {
        $($tt)*
    }
    .parse::<::proc_macro::TokenStream>()
    .unwrap()
)} use ts;

trait CollectVec : IntoIterator + Sized {
    fn vec (self: Self)
      -> Vec<Self::Item>
    {
        impl<T : IntoIterator> CollectVec for T {}

        self.into_iter().collect()
    }
}
