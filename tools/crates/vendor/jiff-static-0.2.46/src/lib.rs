/*!
This crate provides macros for defining `static` data structures for Jiff.

The macros in this crate are re-exported in the [`jiff::tz`] sub-module.
Users should _not_ depend on this crate directly or import from it. Instead,
enable the `static` or `static-tz` features of Jiff and use the re-exports in
`jiff::tz`.

At present, the macros in this crate are limited to creating `TimeZone`
in a `const` context. This works by reading TZif data (e.g., from
`/usr/share/zoneinfo/America/New_York` or from [`jiff-tzdb`]) at compile
time and generating Rust source code that builds a `TimeZone`.

# Documentation

The macros defined in this crate are documented on their corresponding
re-exports in Jiff:

* `get` is documented at [`jiff::tz::get`].
* `include` is documented at [`jiff::tz::include`].

# Compatibility

The APIs required to build a `TimeZone` in a `const` context are exposed by
Jiff but not part of Jiff's public API for the purposes of semver (and do not
appear in `rustdoc`). The only guarantee provided by `jiff` and `jiff-static`
is that there is exactly one version of `jiff` that `jiff-static` works with.
Conventionally, this is indicated by the exact same version string. That is,
`jiff-static 0.2.2` is only guaranteed to work with `jiff 0.2.2`.

This compatibility constraint is managed by Jiff, so that you should never
need to worry about it. In particular, users should never directly depend on
this crate. Everything should be managed through the `jiff` crate.

[`jiff-tzdb`]: https://docs.rs/jiff-tzdb
[`jiff::tz`]: https://docs.rs/jiff/0.2/jiff/tz/index.html
[`jiff::tz::get`]: https://docs.rs/jiff/0.2/jiff/tz/macro.get.html
[`jiff::tz::include`]: https://docs.rs/jiff/0.2/jiff/tz/macro.include.html
*/

extern crate alloc;
extern crate proc_macro;

use proc_macro::TokenStream;
use quote::quote;

use jcore::tz::{posix, tzif, TimeZoneId};

// Public API docs are in Jiff.
#[proc_macro]
pub fn include(input: TokenStream) -> TokenStream {
    let input = syn::parse_macro_input!(input as Include);
    proc_macro::TokenStream::from(input.quote())
}

// Public API docs are in Jiff.
#[cfg(feature = "tzdb")]
#[proc_macro]
pub fn get(input: TokenStream) -> TokenStream {
    let input = syn::parse_macro_input!(input as Get);
    proc_macro::TokenStream::from(input.quote())
}

/// The entry point for the `include!` macro.
#[derive(Debug)]
struct Include {
    tzif: tzif::MaybeNamedTimeZone,
}

impl Include {
    fn from_path_only(path: &str) -> Result<Include, String> {
        const NEEDLE: &str = "zoneinfo/";

        let Some(zoneinfo) = path.rfind(NEEDLE) else {
            return Err(format!(
                "could not extract IANA time zone identifier from \
                 file path `{path}` \
                 (could not find `zoneinfo` in path), \
                 please provide IANA time zone identifier as second \
                 parameter",
            ));
        };
        let idstart = zoneinfo.saturating_add(NEEDLE.len());
        let id = &path[idstart..];
        Include::from_path_with_id(id, path)
    }

    fn from_path_with_id(id: &str, path: &str) -> Result<Include, String> {
        let id = TimeZoneId::new_or_heap(id);
        let data = std::fs::read(path)
            .map_err(|e| format!("failed to read {path}: {e}"))?;
        let tzif = tzif::TimeZone::parse(&data)
            .map_err(|e| {
                format!("failed to parse TZif data from {path}: {e}")
            })?
            .into_named(id);
        Ok(Include { tzif })
    }

    fn quote(&self) -> proc_macro2::TokenStream {
        self.tzif.quote()
    }
}

impl syn::parse::Parse for Include {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Include> {
        let lit1 = input.parse::<syn::LitStr>()?.value();
        if !input.lookahead1().peek(syn::Token![,]) {
            return Ok(
                Include::from_path_only(&lit1).map_err(|e| input.error(e))?
            );
        }
        input.parse::<syn::Token![,]>()?;
        if input.is_empty() {
            return Ok(
                Include::from_path_only(&lit1).map_err(|e| input.error(e))?
            );
        }
        let lit2 = input.parse::<syn::LitStr>()?.value();
        // Permit optional trailing comma.
        if input.lookahead1().peek(syn::Token![,]) {
            input.parse::<syn::Token![,]>()?;
        }
        Ok(Include::from_path_with_id(&lit2, &lit1)
            .map_err(|e| input.error(e))?)
    }
}

/// The entry point for the `get!` macro.
#[cfg(feature = "tzdb")]
#[derive(Debug)]
struct Get {
    tzif: tzif::MaybeNamedTimeZone,
}

#[cfg(feature = "tzdb")]
impl Get {
    fn from_id(id: &str) -> Result<Get, String> {
        let (id, data) = jiff_tzdb::get(id).ok_or_else(|| {
            format!("could not find time zone `{id}` in bundled tzdb")
        })?;
        let id = TimeZoneId::statik(id);
        let tzif = tzif::TimeZone::parse(&data)
            .map_err(|e| {
                format!("failed to parse TZif data from bundled `{id}`: {e}")
            })?
            .into_named(id);
        Ok(Get { tzif })
    }

    fn quote(&self) -> proc_macro2::TokenStream {
        self.tzif.quote()
    }
}

#[cfg(feature = "tzdb")]
impl syn::parse::Parse for Get {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Get> {
        let lit1 = input.parse::<syn::LitStr>()?.value();
        if input.lookahead1().peek(syn::Token![,]) {
            input.parse::<syn::Token![,]>()?;
        }
        Ok(Get::from_id(&lit1).map_err(|e| input.error(e))?)
    }
}

// Everything below at this point is quasi-quoting the `shared` data type
// values into `static` data structures as Rust source code.

trait Quote {
    fn quote(&self) -> proc_macro2::TokenStream;
}

impl Quote for tzif::MaybeNamedTimeZone {
    fn quote(&self) -> proc_macro2::TokenStream {
        let tzif::MaybeNamedTimeZone { ref name, ref tz } = *self;
        let tzif::TimeZone {
            version,
            checksum,
            ref designations,
            ref posix_tz,
            ref types,
            ref transitions,
        } = *tz;
        // We are guaranteed to always have a name in this context.
        let name = name.as_ref().unwrap().quote();
        let designations = designations.iter().map(Quote::quote);
        let posix_tz = posix_tz
            .as_ref()
            .map(|tz| {
                let tz = tz.quote();
                quote!(Some(#tz))
            })
            .unwrap_or_else(|| quote!(None));
        let types = types.iter().map(tzif::LocalTimeType::quote);
        let transitions = transitions.quote();
        quote! {{
            static __TZ_DESIGNATIONS: &[jiff::__jcore::tz::Abbreviation] = &[#(#designations),*];
            static __TZ_INTERNAL: jiff::__jcore::tz::tzif::MaybeNamedTimeZone =
                jiff::__jcore::tz::tzif::MaybeNamedTimeZone {
                    name: Some(#name),
                    tz: jiff::__jcore::tz::tzif::TimeZone {
                        version: #version,
                        checksum: #checksum,
                        designations: jiff::__jcore::util::MaybeStaticSlice::statik(__TZ_DESIGNATIONS),
                        posix_tz: #posix_tz,
                        types: jiff::__jcore::util::MaybeStaticSlice::statik(&[#(#types),*]),
                        transitions: #transitions,
                    },
                };
            static TZ: jiff::tz::TimeZone =
                jiff::tz::TimeZone::__internal_from_tzif(&__TZ_INTERNAL);
            // SAFETY: Since we are guaranteed that the `TimeZone` is
            // constructed above as a static TZif time zone, it follows
            // that it is safe to memcpy's its internal representation.
            //
            // NOTE: We arrange things this way so that `jiff::tz::get!`
            // can be used "by value" in most contexts. Basically, we
            // "pin" the time zone to a static so that it has a guaranteed
            // static lifetime. Otherwise, since `TimeZone` has a `Drop`
            // impl, it's easy to run afoul of this and have it be dropped
            // earlier than you like. Since this particular variant of
            // `TimeZone` can always be memcpy'd internally, we just do
            // this dance here to save the user from having to write out
            // their own `static`.
            //
            // NOTE: It would be nice if we could make this `copy` routine
            // safe, or at least panic if it's misused. But to do that, you
            // need to know the time zone variant. And to know the time
            // zone variant, you need to "look" at the tag in the pointer.
            // And looking at the address of a pointer in a `const` context
            // is precarious.
            unsafe { TZ.copy() }
        }}
    }
}

impl Quote for tzif::Transitions {
    fn quote(&self) -> proc_macro2::TokenStream {
        let tzif::Transitions {
            ref timestamps,
            ref civil_starts,
            ref civil_ends,
            ref infos,
        } = *self;
        let timestamps = timestamps.iter().map(tzif::Timestamp::quote);
        let civil_starts = civil_starts.iter().map(tzif::DateTime::quote);
        let civil_ends = civil_ends.iter().map(tzif::DateTime::quote);
        let infos = infos.iter().map(tzif::TransitionInfo::quote);
        quote! {
            jiff::__jcore::tz::tzif::Transitions {
                timestamps: jiff::__jcore::util::MaybeStaticSlice::statik(&[#(#timestamps),*]),
                civil_starts: jiff::__jcore::util::MaybeStaticSlice::statik(&[#(#civil_starts),*]),
                civil_ends: jiff::__jcore::util::MaybeStaticSlice::statik(&[#(#civil_ends),*]),
                infos: jiff::__jcore::util::MaybeStaticSlice::statik(&[#(#infos),*]),
            }
        }
    }
}

impl Quote for tzif::LocalTimeType {
    fn quote(&self) -> proc_macro2::TokenStream {
        let tzif::LocalTimeType { offset, dst, designation, indicator } =
            *self;
        let offset = offset.quote();
        let dst = dst.quote();
        let indicator = indicator.quote();
        quote! {
            jiff::__jcore::tz::tzif::LocalTimeType {
                offset: #offset,
                dst: #dst,
                designation: #designation,
                indicator: #indicator,
            }
        }
    }
}

impl Quote for tzif::Indicator {
    fn quote(&self) -> proc_macro2::TokenStream {
        match *self {
            tzif::Indicator::LocalWall => quote! {
                jiff::__jcore::tz::tzif::Indicator::LocalWall
            },
            tzif::Indicator::LocalStandard => quote! {
                jiff::__jcore::tz::tzif::Indicator::LocalStandard
            },
            tzif::Indicator::UTStandard => quote! {
                jiff::__jcore::tz::tzif::Indicator::UTStandard
            },
        }
    }
}

impl Quote for tzif::TransitionInfo {
    fn quote(&self) -> proc_macro2::TokenStream {
        let tzif::TransitionInfo { type_index, kind } = *self;
        let kind = kind.quote();
        quote! {
            jiff::__jcore::tz::tzif::TransitionInfo {
                type_index: #type_index,
                kind: #kind,
            }
        }
    }
}

impl Quote for tzif::TransitionKind {
    fn quote(&self) -> proc_macro2::TokenStream {
        match *self {
            tzif::TransitionKind::Unambiguous => quote! {
                jiff::__jcore::tz::tzif::TransitionKind::Unambiguous
            },
            tzif::TransitionKind::Gap => quote! {
                jiff::__jcore::tz::tzif::TransitionKind::Gap
            },
            tzif::TransitionKind::Fold => quote! {
                jiff::__jcore::tz::tzif::TransitionKind::Fold
            },
        }
    }
}

impl Quote for tzif::DateTime {
    fn quote(&self) -> proc_macro2::TokenStream {
        let year = self.year();
        let month = self.month();
        let day = self.day();
        let hour = self.hour();
        let minute = self.minute();
        let second = self.second();
        quote! {
            jiff::__jcore::tz::tzif::DateTime::new(jiff::__jcore::civil::datetime(
                #year,
                #month,
                #day,
                #hour,
                #minute,
                #second,
                0
            ))
        }
    }
}

impl Quote for posix::TimeZone {
    fn quote(&self) -> proc_macro2::TokenStream {
        let posix::TimeZone { ref std_abbrev, ref std_offset, ref dst } =
            *self;
        let std_abbrev = std_abbrev.quote();
        let std_offset = std_offset.quote();
        let dst = dst
            .as_ref()
            .map(|dst| {
                let dst = dst.quote();
                quote!(Some(#dst))
            })
            .unwrap_or_else(|| quote!(None));
        quote! {
            jiff::__jcore::tz::posix::TimeZone {
                std_abbrev: #std_abbrev,
                std_offset: #std_offset,
                dst: #dst,
            }
        }
    }
}

impl Quote for posix::Dst {
    fn quote(&self) -> proc_macro2::TokenStream {
        let posix::Dst { ref abbrev, ref offset, ref rule } = *self;
        let abbrev = abbrev.quote();
        let offset = offset.quote();
        let rule = rule.quote();
        quote! {
            jiff::__jcore::tz::posix::Dst {
                abbrev: #abbrev,
                offset: #offset,
                rule: #rule,
            }
        }
    }
}

impl Quote for posix::Rule {
    fn quote(&self) -> proc_macro2::TokenStream {
        let start = self.start.quote();
        let end = self.end.quote();
        quote! {
            jiff::__jcore::tz::posix::Rule { start: #start, end: #end }
        }
    }
}

impl Quote for posix::DayTime {
    fn quote(&self) -> proc_macro2::TokenStream {
        let posix::DayTime { ref date, ref time } = *self;
        let date = date.quote();
        let time = time.quote();
        quote! {
            jiff::__jcore::tz::posix::DayTime { date: #date, time: #time }
        }
    }
}

impl Quote for posix::Day {
    fn quote(&self) -> proc_macro2::TokenStream {
        match *self {
            posix::Day::JulianOne(day) => quote! {
                jiff::__jcore::tz::posix::Day::JulianOne(#day)
            },
            posix::Day::JulianZero(day) => quote! {
                jiff::__jcore::tz::posix::Day::JulianZero(#day)
            },
            posix::Day::WeekdayOfMonth { month, week, weekday } => {
                let weekday = weekday.quote();
                quote! {
                    jiff::__jcore::tz::posix::Day::WeekdayOfMonth {
                        month: #month,
                        week: #week,
                        weekday: #weekday,
                    }
                }
            }
        }
    }
}

impl Quote for jcore::civil::Weekday {
    fn quote(&self) -> proc_macro2::TokenStream {
        use jcore::civil::Weekday::*;
        match *self {
            Sunday => quote!(jiff::__jcore::civil::Weekday::Sunday),
            Monday => quote!(jiff::__jcore::civil::Weekday::Monday),
            Tuesday => quote!(jiff::__jcore::civil::Weekday::Tuesday),
            Wednesday => quote!(jiff::__jcore::civil::Weekday::Wednesday),
            Thursday => quote!(jiff::__jcore::civil::Weekday::Thursday),
            Friday => quote!(jiff::__jcore::civil::Weekday::Friday),
            Saturday => quote!(jiff::__jcore::civil::Weekday::Saturday),
        }
    }
}

impl Quote for jcore::tz::posix::TransitionCivilTime {
    fn quote(&self) -> proc_macro2::TokenStream {
        let posix::TransitionCivilTime { second } = *self;
        quote! {
            jiff::__jcore::tz::posix::TransitionCivilTime { second: #second }
        }
    }
}

impl Quote for tzif::Timestamp {
    fn quote(&self) -> proc_macro2::TokenStream {
        let second = self.as_second();
        quote! {
            jiff::__jcore::tz::tzif::Timestamp::new(
                jiff::__jcore::Timestamp::constant(#second, 0),
            )
        }
    }
}

impl Quote for jcore::tz::Offset {
    fn quote(&self) -> proc_macro2::TokenStream {
        let seconds = self.seconds();
        quote! {
            jiff::__jcore::tz::Offset::constant_seconds(#seconds)
        }
    }
}

impl Quote for jcore::tz::Dst {
    fn quote(&self) -> proc_macro2::TokenStream {
        match *self {
            jcore::tz::Dst::Yes => quote! { jiff::__jcore::tz::Dst::Yes },
            jcore::tz::Dst::No => quote! { jiff::__jcore::tz::Dst::No },
        }
    }
}

impl<const N: usize> Quote for jcore::util::SmallStr<N> {
    fn quote(&self) -> proc_macro2::TokenStream {
        let s = self.as_str();
        quote! {
            jiff::__jcore::util::SmallStr::statik(#s)
        }
    }
}

impl<T: Quote + 'static> Quote for jcore::util::MaybeStaticSlice<T> {
    fn quote(&self) -> proc_macro2::TokenStream {
        let slice = self.as_slice().iter().map(Quote::quote);
        quote! {{
            jiff::__jcore::util::MaybeStaticSlice::statik(&[#(#slice),*])
        }}
    }
}
