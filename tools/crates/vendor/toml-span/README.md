<!-- Allow this file to not have a first line heading -->
<!-- markdownlint-disable-file MD041 no-emphasis-as-heading no-duplicate-heading -->

<!-- inline html -->
<!-- markdownlint-disable-file MD033 -->

<div align="center">

# `↔️ toml-span`

**Span-preserving toml deserializer**

[![Embark](https://img.shields.io/badge/embark-open%20source-blueviolet.svg)](https://embark.dev)
[![Embark](https://img.shields.io/badge/discord-ark-%237289da.svg?logo=discord)](https://discord.gg/dAuKfZS)
[![Crates.io](https://img.shields.io/crates/v/rust-gpu.svg)](https://crates.io/crates/toml-span)
[![Docs](https://docs.rs/toml-span/badge.svg)](https://docs.rs/toml-span)
[![dependency status](https://deps.rs/repo/github/EmbarkStudios/toml-span/status.svg)](https://deps.rs/repo/github/EmbarkStudios/toml-span)
[![Build status](https://github.com/EmbarkStudios/toml-span/workflows/CI/badge.svg)](https://github.com/EmbarkStudios/toml-span/actions)
</div>

## Differences from `toml`

First off I just want to be up front and clear about the differences/limitations of this crate versus `toml`

1. No `serde` support for deserialization, there is a `serde` feature, but that only enables serialization of the `Value` and `Spanned` types.
1. No toml serialization. This crate is only intended to be a span preserving deserializer, there is no intention to provide serialization to toml, especially the advanced format preserving kind provided by `toml-edit`.
1. No datetime deserialization. It would be trivial to add support for this (behind an optional feature), I just have no use for it at the moment. PRs welcome.

## Why does this crate exist?

### The problem

This crate was specifically made to suit the needs of [cargo-deny], namely, that it can always retrieve the span of any toml item that it wants to. While the [toml](https://docs.rs/toml/latest/toml/) crate can also produce span information via [toml::Spanned](https://docs.rs/toml/latest/toml/struct.Spanned.html) there is one rather significant limitation, namely, that it must pass through [serde](https://docs.rs/serde/latest/serde/). While in simple cases the `Spanned` type works quite well, eg.

```rust,ignore
#[derive(serde::Deserialize)]
struct Simple {
    /// This works just fine
    simple_string: toml::Spanned<String>,
}
```

As soon as you have a [more complicated scenario](https://play.rust-lang.org/?version=nightly&mode=debug&edition=2021&gist=aeb611bbe387538d2ebb6780055b3167), the mechanism that `toml` uses to get the span information breaks down.

```rust,ignore
#[derive(serde::Deserialize)]
#[serde(untagged)]
enum Ohno {
    Integer(u32),
    SpannedString(toml::Spanned<String>),
}

#[derive(serde::Deserialize)]
struct Root {
    integer: Ohno,
    string: Ohno
}

fn main() {
    let toml = r#"
integer = 42
string = "we want this to be spanned"
"#;

    let parsed: Root = toml::from_str(toml).expect("failed to deserialize toml");
}
```

```text
thread 'main' panicked at src/main.rs:20:45:
failed to deserialize toml: Error { inner: Error { inner: TomlError { message: "data did not match any variant of untagged enum Ohno", original: Some("\ninteger = 42\nstring = \"we want this to be spanned\"\n"), keys: ["string"], span: Some(23..51) } } }
```

To understand why this fails we can look at what `#[derive(serde::Deserialize)]` expand to for `Ohno` in HIR.

```rust,ignore
#[allow(unused_extern_crates, clippy :: useless_attribute)]
extern crate serde as _serde;
#[automatically_derived]
impl <'de> _serde::Deserialize<'de> for Ohno {
    fn deserialize<__D>(__deserializer: __D)
        -> _serde::__private::Result<Self, __D::Error> where
        __D: _serde::Deserializer<'de> {
            let __content =
                match #[lang = "branch"](<_serde::__private::de::Content as
                                        _serde::Deserialize>::deserialize(__deserializer)) {
                        #[lang = "Break"] {  0: residual } =>
                            #[allow(unreachable_code)]
                            return #[lang = "from_residual"](residual),
                        #[lang = "Continue"] {  0: val } =>
                            #[allow(unreachable_code)]
                            val,
                    };
            let __deserializer =
                _serde::__private::de::ContentRefDeserializer<, ,
                        __D::Error>::new(&__content);
            if let _serde::__private::Ok(__ok) =
                        _serde::__private::Result::map(<u32 as
                                    _serde::Deserialize>::deserialize(__deserializer),
                            Ohno::Integer) { return _serde::__private::Ok(__ok); }
                    if let _serde::__private::Ok(__ok) =
                                _serde::__private::Result::map(<toml::Spanned<String> as
                                            _serde::Deserialize>::deserialize(__deserializer),
                                    Ohno::SpannedString) { return _serde::__private::Ok(__ok); }
                            _serde::__private::Err(_serde::de::Error::custom("data did not match any variant of untagged enum Ohno"))
    }
}
```

What serde does in the untagged case is first deserialize into `_serde::__private::de::Content`, an internal API container that is easiest to think of as something like `serde_json::Value`. This is because serde speculatively parses each enum variant until one succeeds by passing a `ContentRefDeserializer` that just borrows the deserialized `Content` from earlier to satisfy the serde deserialize API consuming the `Deserializer`. The problem comes because of how [`toml::Spanned`](https://docs.rs/serde_spanned/0.6.5/src/serde_spanned/spanned.rs.html#161-212) works, namely that it uses a hack to workaround the limitations of the serde API in order to "deserialize" the item as well as its span information, by the `Spanned` object specifically requesting a set of keys from the `toml::Deserializer` impl so that it can [encode](https://github.com/toml-rs/toml/blob/c4b62fda23343037ebe5ea93db9393cb25fcf233/crates/toml_edit/src/de/spanned.rs#L27-L70) the span information as if it was a struct to satisfy serde. But serde doesn't know that when it deserializes the `Content` object, it just knows that the Deserializer reports it has a string, int or what have you, and deserializes that, "losing" the span information. This problem also affects things like `#[serde(flatten)]` for slightly different reasons, but they all basically come down to the serde API not truly supporting span information, nor [any plans](https://github.com/serde-rs/serde/issues/1811) to.

### How `toml-span` is different

This crate works by just...not using `serde`. The core of the crate is based off of [basic-toml](https://github.com/dtolnay/basic-toml) which itself a fork of `toml v0.5` before it added a ton of features an complexity that...well, is not needed by [cargo-deny] or many other crates that only need deserialization.

Removing `serde` support means that while deserialization must be manually written, which can be tedious in some cases, while doing the porting of [cargo-deny] I actually came to appreciate it more and more due to a couple of things.

1. Maximal control. `toml-span` does an initial deserialization pass into `toml_span::value::Value` which keeps span information for both keys and values, and provides helpers (namely `TableHelper`), but other than satisfying the `toml_span::Deserialize` trait doesn't restrict you in how you want to deserialize your values, and you don't even have to use that if you don't want to.
2. While it's slower to manually write deserialization code rather than just putting on a few serde attributes, the truth is that that initial convenience carries a compile time cost in terms of `serde_derive` and all of its dependencies, as well as all of the code that is generated, for...ever. This is fine when you are prototyping, but becomes quite wasteful once you have (mostly/somewhat) stabilized your data format.
3. (optional) Span-based errors. `toml-span` provides the `reporting` feature that can be enabled to have `toml_span::Error` be able to be converted into a [Diagnostic](https://docs.rs/codespan-reporting/latest/codespan_reporting/diagnostic/struct.Diagnostic.html) which can provide nice error output if you use the `codespan-reporting` crate.

## Usage

### Simple

The most simple use case for `toml-span` is just as slimmer version of `toml` that also has a pointer API similar to [serde_json](https://docs.rs/serde_json/latest/serde_json/enum.Value.html#method.pointer) allowing easy piecemeal deserialization of a toml document.

#### `toml` version

```rust,ignore
fn is_crates_io_sparse(config: &toml::Value) -> Option<bool> {
    config
        .get("registries")
        .and_then(|v| v.get("crates-io"))
        .and_then(|v| v.get("protocol"))
        .and_then(|v| v.as_str())
        .and_then(|v| match v {
            "sparse" => Some(true),
            "git" => Some(false),
            _ => None,
        })
}
```

#### `toml-span` version

```rust
fn is_crates_io_sparse(config: &toml_span::Value) -> Option<bool> {
    match config.pointer("/registries/crates-io/protocol").and_then(|p| p.as_str())? {
        "sparse" => Some(true),
        "git" => Some(false),
        _ => None
    }
}
```

### Common

Of course the most common case is deserializing toml into Rust containers.

#### `toml` version

```rust,ignore
#[derive(Deserialize, Clone)]
#[cfg_attr(test, derive(Debug, PartialEq, Eq))]
#[serde(rename_all = "kebab-case", deny_unknown_fields)]
pub struct CrateBan {
    pub name: Spanned<String>,
    pub version: Option<VersionReq>,
    /// One or more crates that will allow this crate to be used if it is a
    /// direct dependency
    pub wrappers: Option<Spanned<Vec<Spanned<String>>>>,
    /// Setting this to true will only emit an error if multiple
    // versions of the crate are found
    pub deny_multiple_versions: Option<Spanned<bool>>,
}
```

#### `toml-span` version

The following code is much more verbose (before proc macros run at least), but show cases something that moving [cargo-deny] to `toml-span` allowed, namely, `PackageSpec`.

Before `toml-span`, all cases where a user specifies a crate spec, (ie, name + optional version requirement) was done via two separate fields, `name` and `version`. This was quite verbose, as in many cases not only is `version` not specified, but also could be just a string if the user doesn't need/want to provide other fields. Normally one would use the [string or struct](https://serde.rs/string-or-struct.html) idiom but this was impossible due to how I wanted to reorganize the data to have the package spec as either a string or struct, _as well as_ optional data that is flattened to the same level as the package spec. But since `toml-span` changes how deserialization is done, this change was quite trivial after the initial work of getting the crate stood up was done.

```rust,ignore
pub type CrateBan = PackageSpecOrExtended<CrateBanExtended>;

#[cfg_attr(test, derive(Debug, PartialEq, Eq))]
pub struct CrateBanExtended {
    /// One or more crates that will allow this crate to be used if it is a
    /// direct dependency
    pub wrappers: Option<Spanned<Vec<Spanned<String>>>>,
    /// Setting this to true will only emit an error if multiple versions of the
    /// crate are found
    pub deny_multiple_versions: Option<Spanned<bool>>,
    /// The reason for banning the crate
    pub reason: Option<Reason>,
    /// The crate to use instead of the banned crate, could be just the crate name
    /// or a URL
    pub use_instead: Option<Spanned<String>>,
}

impl<'de> Deserialize<'de> for CrateBanExtended {
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        // The table helper provides convenience wrappers around a Value::Table, which
        // is just a BTreeMap<Key, Value>
        let mut th = TableHelper::new(value)?;

        // Since we specify the keys manually there is no need for serde(rename/rename_all)
        let wrappers = th.optional("wrappers");
        let deny_multiple_versions = th.optional("deny-multiple-versions");
        let reason = th.optional_s("reason");
        let use_instead = th.optional("use-instead");
        // Specifying None means that any keys that still exist in the table are
        // unknown, producing an error the same as with serde(deny_unknown_fields)
        th.finalize(None)?;

        Ok(Self {
            wrappers,
            deny_multiple_versions,
            reason: reason.map(Reason::from),
            use_instead,
        })
    }
}

#[derive(Clone, PartialEq, Eq)]
pub struct PackageSpec {
    pub name: Spanned<String>,
    pub version_req: Option<VersionReq>,
}

impl<'de> Deserialize<'de> for PackageSpec {
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        use std::borrow::Cow;

        struct Ctx<'de> {
            inner: Cow<'de, str>,
            split: Option<(usize, bool)>,
            span: Span,
        }

        impl<'de> Ctx<'de> {
            fn from_str(bs: Cow<'de, str>, span: Span) -> Self {
                let split = bs
                    .find('@')
                    .map(|i| (i, true))
                    .or_else(|| bs.find(':').map(|i| (i, false)));
                Self {
                    inner: bs,
                    split,
                    span,
                }
            }
        }

        let ctx = match value.take() {
            ValueInner::String(s) => Ctx::from_str(s, value.span),
            ValueInner::Table(tab) => {
                let mut th = TableHelper::from((tab, value.span));

                if let Some(mut val) = th.table.remove(&"crate".into()) {
                    let s = val.take_string(Some("a crate spec"))?;
                    th.finalize(Some(value))?;

                    Ctx::from_str(s, val.span)
                } else {
                    // Encourage user to use the 'crate' spec instead
                    let name = th.required("name").map_err(|e| {
                        if matches!(e.kind, toml_span::ErrorKind::MissingField(_)) {
                            (toml_span::ErrorKind::MissingField("crate"), e.span).into()
                        } else {
                            e
                        }
                    })?;
                    let version = th.optional::<Spanned<Cow<'_, str>>>("version");

                    // We return all the keys we haven't deserialized back to the value,
                    // so that further deserializers can use them as this spec is
                    // always embedded in a larger structure
                    th.finalize(Some(value))?;

                    let version_req = if let Some(vr) = version {
                        Some(vr.value.parse().map_err(|e: semver::Error| {
                            toml_span::Error::from((
                                toml_span::ErrorKind::Custom(e.to_string()),
                                vr.span,
                            ))
                        })?)
                    } else {
                        None
                    };

                    return Ok(Self { name, version_req });
                }
            }
            other => return Err(expected("a string or table", other, value.span).into()),
        };

        let (name, version_req) = if let Some((i, make_exact)) = ctx.split {
            let mut v: VersionReq = ctx.inner[i + 1..].parse().map_err(|e: semver::Error| {
                toml_span::Error::from((
                    toml_span::ErrorKind::Custom(e.to_string()),
                    Span::new(ctx.span.start + i + 1, ctx.span.end),
                ))
            })?;
            if make_exact {
                if let Some(comp) = v.comparators.get_mut(0) {
                    comp.op = semver::Op::Exact;
                }
            }

            (
                Spanned::with_span(
                    ctx.inner[..i].into(),
                    Span::new(ctx.span.start, ctx.span.start + i),
                ),
                Some(v),
            )
        } else {
            (Spanned::with_span(ctx.inner.into(), ctx.span), None)
        };

        Ok(Self { name, version_req })
    }
}

pub struct PackageSpecOrExtended<T> {
    pub spec: PackageSpec,
    pub inner: Option<T>,
}

impl<T> PackageSpecOrExtended<T> {
    pub fn try_convert<V, E>(self) -> Result<PackageSpecOrExtended<V>, E>
    where
        V: TryFrom<T, Error = E>,
    {
        let inner = if let Some(i) = self.inner {
            Some(V::try_from(i)?)
        } else {
            None
        };

        Ok(PackageSpecOrExtended {
            spec: self.spec,
            inner,
        })
    }

    pub fn convert<V>(self) -> PackageSpecOrExtended<V>
    where
        V: From<T>,
    {
        PackageSpecOrExtended {
            spec: self.spec,
            inner: self.inner.map(V::from),
        }
    }
}

impl<'de, T> toml_span::Deserialize<'de> for PackageSpecOrExtended<T>
where
    T: toml_span::Deserialize<'de>,
{
    fn deserialize(value: &mut Value<'de>) -> Result<Self, DeserError> {
        let spec = PackageSpec::deserialize(value)?;

        // If more keys exist in the table (or string) then try to deserialize
        // the rest as the "extended" portion
        let inner = if value.has_keys() {
            Some(T::deserialize(value)?)
        } else {
            None
        };

        Ok(Self { spec, inner })
    }
}
```

## Contributing

[![Contributor Covenant](https://img.shields.io/badge/contributor%20covenant-v1.4-ff69b4.svg)](CODE_OF_CONDUCT.md)

We welcome community contributions to this project.

Please read our [Contributor Guide](CONTRIBUTING.md) for more information on how to get started.
Please also read our [Contributor Terms](CONTRIBUTING.md#contributor-terms) before you make any contributions.

Any contribution intentionally submitted for inclusion in an Embark Studios project, shall comply with the Rust standard licensing model (MIT OR Apache 2.0) and therefore be dual licensed as described below, without any additional terms or conditions:

### License

This contribution is dual licensed under EITHER OF

- Apache License, Version 2.0, ([LICENSE-APACHE](LICENSE-APACHE) or <http://www.apache.org/licenses/LICENSE-2.0>)
- MIT license ([LICENSE-MIT](LICENSE-MIT) or <http://opensource.org/licenses/MIT>)

at your option.

For clarity, "your" refers to Embark or any other licensee/user of the contribution.

[cargo-deny]: https://github.com/EmbarkStudios/cargo-deny
