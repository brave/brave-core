# CHANGELOG

0.1.29 (2025-02-02)
===================
This release includes a few small enhancements and a bug fix. In particular,
there is now Serde support for `TimeZone` and the `ISOWeekDate` API has been
filled out a bit more.

Unless a serious issue is uncovered, my plan is that this will be the last
release before `jiff 0.2`.

Enhancements:

* [#89](https://github.com/BurntSushi/jiff/issues/89):
Opt-in support for using Serde with `jiff::tz::TimeZone` has been added.
* [#227](https://github.com/BurntSushi/jiff/issues/227):
The `civil::ISOWeekDate` API has been beefed up with a few convenience methods.
* [#233](https://github.com/BurntSushi/jiff/issues/233):
Add `tz::Offset::round` for rounding time zone offsets.

Bug fixes:

* [#231](https://github.com/BurntSushi/jiff/issues/231):
Use more flexible offset equality when parsing offsets with fractional minutes.


0.1.28 (2025-01-27)
===================
This is a small release that just removes the dev-dependency on `serde_yml`.
It has been replaced with the deprecated `serde_yaml`. See
[this post about `serde_yml` shenanigans](https://x.com/davidtolnay/status/1883906113428676938)
for why this was done. Note that this was only a dev-dependency and thus doesn't
impact folks using Jiff.

Bug fixes:

* [#225](https://github.com/BurntSushi/jiff/pull/225):
Remove dependency on `serde_yml` in favor of `serde_yaml`.


0.1.27 (2025-01-25)
===================
This is a small release with a bug fix for precision loss in some cases when
doing arithmetic on `Timestamp` or `Zoned`.

Bug fixes:

* [#223](https://github.com/BurntSushi/jiff/issues/223):
Fix the check for fractional seconds before taking the fast path.


0.1.26 (2025-01-23)
===================
This is a small release with another deprecation and a new API for doing
prefix parsing via `strptime`. There's also a bug fix for a corner case
when dealing with daylight saving time gaps with the `Zoned::with` API.

Deprecations:

* [#210](https://github.com/BurntSushi/jiff/pull/210):
  Deprecate `ISOWeekDate::to_date` in favor of `ISOWeekDate::date`.

Enhancements:

* [#209](https://github.com/BurntSushi/jiff/issues/209):
  Add `fmt::strtime::BrokenDownTime::parse_prefix` for parsing only a prefix.

Bug fixes:

* [#211](https://github.com/BurntSushi/jiff/issues/211):
  Fix unintuitive behavior of `Zoned::with` when time falls in a DST gap.


0.1.25 (2025-01-21)
===================
This release contains a number of deprecations in preparation for a `jiff 0.2`
release. The deprecations are meant to facilitate a smoother transition. The
deprecations, when possible, come with new APIs that will permit users to write
forward compatible code that will work in both `jiff 0.1` and `jiff 0.2`.

This release also includes a handful of new conversion specifiers in Jiff's
`strftime` and `strptime` APIs. This improves compatibility with the analogous
implementation with GNU libc.

Deprecations:

* [#28](https://github.com/BurntSushi/jiff/issues/28):
The `intz` methods on `Zoned`, `Timestamp`, `civil::DateTime` and `civil::Date`
have been deprecated in favor of `in_tz`.
* [#32](https://github.com/BurntSushi/jiff/issues/32):
The `Eq` and `PartialEq` trait implementations on `Span` have been deprecated
in favor of using the new `SpanFieldwise` type.
* [#48](https://github.com/BurntSushi/jiff/issues/48):
Silently assuming days are always 24 hours in some `Span` APIs has now been
deprecated. This will become an error in `jiff 0.2`. To continue assuming
days are 24 hours without a relative reference date, you can use the new
`SpanRelativeTo::days_are_24_hours` API. In `jiff 0.1`, you'll seen a
WARN-level log message emitted if you're code will be broken by `jiff 0.2`.
* [#147](https://github.com/BurntSushi/jiff/issues/147):
Both `%V` and `%:V` have been deprecated in favor of `%Q` and `%:Q`. In
`jiff 0.2`, `%V` will correspond to the ISO 8601 week number and `%:V` will
result in an error. This change was made to improve compatibility with other
`strtime` implementations. `%V` and `%:V` continue to correspond to IANA
time zone identifiers in `jiff 0.1`, but using them for parsing or formatting
will result in a WARN-level deprecation message.

Enhancements:

* [#147](https://github.com/BurntSushi/jiff/issues/147):
Adds a number of new conversion specifiers to Jiff's `strftime` and
`strptime` APIs. Specifically, `%C`, `%G`, `%g`, `%j`, `%k`, `%l`, `%n`, `%R`,
`%s`, `%t`, `%U`, `%u`, `%W`, `%w`. Their behavior should match the
corresponding specifiers in GNU libc.


0.1.24 (2025-01-16)
===================
This release updates Jiff's bundled copy of the [IANA Time Zone Database] to
`2025a`. See the [`2025a` release announcement] for more details.

Enhancements:

* [#206](https://github.com/BurntSushi/jiff/pull/206):
Update `jiff-tzdb` to `2025a`.

[`2025a` release announcement]: https://lists.iana.org/hyperkitty/list/tz-announce@iana.org/thread/MWII7R3HMCEDNUCIYQKSSTYYR7UWK4OQ/


0.1.23 (2025-01-13)
===================
This release includes some bug fixes, particularly for compilation on
`aarch64-linux-android`. There are also some minor enhancements, such as making
`Zoned::iso_week_date` a convenience function for `civil::Date::iso_week_date`,
in line with similar functions.

My current plan is to make a reasonably quick transition to `jiff 0.2` with a
few pending breaking changes. I will be making some `jiff 0.1` releases with
deprecations in order to make the transition as smooth as possible. If all goes
well with `jiff 0.2`, then my plan is still to do a Jiff 1.0 release in the
Summer of 2025.

Deprecations:

* [#197](https://github.com/BurntSushi/jiff/discussions/197):
`Date::to_iso_week_date` has been deprecated in favor of `Date::iso_week_date`.

Enhancements:

* [#196](https://github.com/BurntSushi/jiff/discussions/196):
Improve ISO week date documentation regarding weekday offsets.
* [#197](https://github.com/BurntSushi/jiff/discussions/197):
Add `Zoned::iso_week_date`, `DateTime::iso_week_date` and
`Date::iso_week_date`.

Bug fixes:

* [#200](https://github.com/BurntSushi/jiff/issues/200):
Fix compilation failure on Android in a Termux shell.
* [#202](https://github.com/BurntSushi/jiff/pull/202):
Re-add license files to crate artifact.


0.1.22 (2025-01-12)
===================
This release adds support for Android. This support means that Jiff will
automatically read its special concatenated time zone database, and will
read the `persist.sys.timezone` property to determine the system's current
time zone.

See [PLATFORM] for more specific information about Android support.

Note that this release also removed all non-essential files (including tests
and test data) for the artifact uploaded to crates.io. If you need or want
these files, please open a new issue.

Enhancements:

* [#140](https://github.com/BurntSushi/jiff/issues/140):
Add support for the Android platform.


0.1.21 (2025-01-04)
===================
This release includes a new API for setting the unit designator label in a
friendly formatted duration for zero-length durations.

Enhancements:

* [#192](https://github.com/BurntSushi/jiff/issues/192):
Add option to the friendly printer for setting the unit when writing a
zero-length duration.


0.1.20 (2025-01-03)
===================
This release inclues a new type, `Pieces`, in the `jiff::fmt::temporal`
sub-module. This exposes the individual components of a parsed Temporal
ISO 8601 datetime string. It allows users of Jiff to circumvent the checks
in the higher level parsing routines that prevent you from shooting yourself
in the foot.

For example, parsing into a `Zoned` will return an error for raw RFC 3339
timestamps like `2025-01-03T22:03-05` because there is no time zone annotation.
Without a time zone, Jiff cannot do time zone aware arithmetic and rounding.
Instead, such a datetime can only be parsed into a `Timestamp`. This lower
level `Pieces` API now permits users of Jiff to parse this string into its
component parts and assemble it into a `Zoned` if they so choose.

Enhancements:

* [#188](https://github.com/BurntSushi/jiff/issues/188):
Add `fmt::temporal::Pieces` for granular datetime parsing and formatting.


0.1.19 (2025-01-02)
===================
This releases includes a UTF-8 related bug fix and a few enhancements.

Firstly, a `Span`'s default `Display` implementation now writes uppercase
unit designator labels. That means you'll get `P1Y2M3DT4H5M6S` instead
of `P1y2m3dT4h5m6s` by default. You can restore previous behavior via
`jiff::fmt::temporal::SpanPrinter::lowercase`. This change was made to improve
interoperability.

Secondly, `SignedDuration` now supports rounding via `SignedDuration::round`.
Note that it only supports rounding time units (hours or smaller). In order to
round with calendar units, you'll still need to use a `Span`.

Enhancements:

* [#130](https://github.com/BurntSushi/jiff/issues/130):
Document value ranges for methods like `year`, `day`, `hour` and so on.
* [#187](https://github.com/BurntSushi/jiff/issues/187):
Add a rounding API (for time units only) on `SignedDuration`.
* [#190](https://github.com/BurntSushi/jiff/issues/190):
`Span` and `SignedDuration` now use uppercase unit designator labels in their
default ISO 8601 `Display` implementation.

Bug fixes:

* [#155](https://github.com/BurntSushi/jiff/issues/155):
Relax `strftime` format strings from ASCII-only to all of UTF-8.


0.1.18 (2024-12-31)
===================
This release includes a few minor enhancements. Namely, the ability to iterate
over time zone transitions (in the future or the past), and some improvements
to failure modes when `Timestamp` and `Span` arithmetic fails.

Enhancements:

* [#144](https://github.com/BurntSushi/jiff/issues/144):
Add APIs for iterating over the transitions of a time zone.
* [#145](https://github.com/BurntSushi/jiff/issues/145):
Improve docs and error messages around fallible `Timestamp` arithmetic.


0.1.17 (2024-12-31)
===================
This release enhances Jiff's support for `no_std` environments by making its
`alloc` feature optional. When `alloc` is disabled, only fixed offset time
zones are supported and error messages are significantly degraded. If you have
core-only use cases for Jiff, I'd love to hear about them on the issue tracker.

Enhancements:

* [#162](https://github.com/BurntSushi/jiff/issues/162):
Support platforms that do not have atomics in `std`.
* [#168](https://github.com/BurntSushi/jiff/issues/168):
Jiff now supports disabling the `alloc` feature, which enables core-only mode.
* [#169](https://github.com/BurntSushi/jiff/issues/169):
Add `TimeZone::to_fixed_offset` for accessing an invariant offset if possible.


0.1.16 (2024-12-26)
===================
This release includes a new `jiff::fmt::friendly` module for formatting and
parsing durations in a more human readable format than what ISO 8601 specifies.
ISO 8601 remains the "default" duration format in Jiff due to its widespread
support. Here are some examples:

```text
40d
40 days
1y1d
1yr 1d
3d4h59m
3 days, 4 hours, 59 minutes
3d 4h 59m
2h30m
2h 30m
1mo
1w
1 week
1w4d
1 wk 4 days
1m
0.0021s
0s
0d
0 days
3 mins 34s 123ms
3 mins 34.123 secs
3 mins 34,123s
1y1mo1d1h1m1.1s
1yr 1mo 1day 1hr 1min 1.1sec
1 year, 1 month, 1 day, 1 hour, 1 minute 1.1 seconds
1 year, 1 month, 1 day, 01:01:01.1
```

To quickly demonstrate this new feature, here's a simple CLI program using
Clap:

```rust,no_run
use clap::Parser;
use jiff::{Span, Zoned};

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    duration: Span,
}

fn main() {
    let args = Args::parse();
    println!("adding duration to now: {}", &Zoned::now() + args.duration);
}
```

And running the program:

```text
$ cargo run -q -- '1 year, 2 months, 5 hours'
adding duration to now: 2026-02-26T18:58:22-05:00[America/New_York]
$ cargo run -q -- 'P1Y2MT5H'  # ISO 8601 durations are supported too!
adding duration to now: 2026-02-26T19:00:57-05:00[America/New_York]
```

With Jiff, you should no longer need to pull in crates like
[`humantime`](https://docs.rs/humantime) and
[`humantime-serde`](https://docs.rs/humantime-serde)
to accomplish a similar task.

While this new format doesn't support any kind of internationalization, the
prevalence of the `humantime` crate suggests there's a desire for something
like this. The "friendly" format is meant to service all the same use cases
as `humantime` does for durations, but in a way that doesn't let you shoot
yourself in the foot.

The new "friendly" format is now the default for the `Debug` implementations
of both `Span` and `SignedDuration`. It's also available via the "alternate"
`Display` implementations for `Span` and `SignedDuration` as well. Moreover,
the `FromStr` trait implementations for both `Span` and `SignedDuration` will
parse _both_ the ISO 8601 duration and this new "friendly" format. Finally,
when `serde` integration is enabled, the `Deserialize` implementations for
`SignedDuration` and `Span` also automatically parse either ISO 8601 or the
friendly format. For serialization, ISO 8601 remains the default, but the
`jiff::fmt::serde` module provides easy to use helpers to switch to the
friendly format.

The `jiff::fmt::friendly` module documentation provides many more details,
including a complete grammar for the format.

Enhancements:

* [#143](https://github.com/BurntSushi/jiff/pull/143):
Add `Hash` implementation for `Zoned` and `Timestamp`.

Bug fixes:

* [#60](https://github.com/BurntSushi/jiff/issues/60):
Use a better `Debug` output format for `SignedDuration` and `Span`.
* [#111](https://github.com/BurntSushi/jiff/issues/111):
Add a new "friendly" duration format.
* [#138](https://github.com/BurntSushi/jiff/issues/138):
Fix deserialization in `serde_yml` and `serde_yaml` crates.
* [#161](https://github.com/BurntSushi/jiff/pull/161):
Fix `serde` dependency configuration so that it builds in no-std mode.


0.1.15 (2024-11-30)
===================
This release fixes a bug where Jiff would sometimes fail to parse TZif files
(found, typically, in `/usr/share/zoneinfo` on Unix systems). This occurred
when the TZif file contained a time zone transition outside the range of Jiff's
`Timestamp` type (which is `-9999-01-01` to `9999-12-31`). The bug fix works by
clamping the out-of-range transitions to Jiff's supported range.

This bug only seems to occur in some environments where their TZif files
contain more extreme values than what is typically found.

Bug fixes:

* [#163](https://github.com/BurntSushi/jiff/issues/163):
Fix a bug where Jiff would fail to parse some TZif files.


0.1.14 (2024-11-01)
===================
This release introduces new APIs to the RFC 2822 printer that explicitly
print timestamps in a format strictly compatible with [RFC 9110].

Enhancements:

* [#151](https://github.com/BurntSushi/jiff/issues/151):
Add `rfc2822::DateTimePrinter::timestamp_to_rfc9110_string` method.


0.1.13 (2024-09-07)
===================
This release introduces a new `jiff::tz::TimeZone::try_system` API. It is like
`TimeZone::system`, but returns an error instead of an automatic fall back to
UTC when the system time zone could not be discovered.

This also includes an update to the bundled [IANA Time Zone Database] to the
`2024b` release in the `jiff-tzdb` crate. As a reminder, the bundled database
is not used or included on Unix platforms by default. See [PLATFORM] for more
details.

Enhancements:

* [#65](https://github.com/BurntSushi/jiff/issues/65):
Add `TimeZone::try_system` for fallibly determining the system's time zone.
* [#125](https://github.com/BurntSushi/jiff/pull/125):
Update to the `2024b` release of the [IANA Time Zone Database].


0.1.12 (2024-08-31)
===================
This release introduces some new minor APIs that support formatting
`Timestamp` values as RFC 3339 strings with a specific offset.

Previously, using the standard formatting routines that Jiff provides, it was
only possible to format a `Timestamp` using Zulu time. For example:

```rust
use jiff::Timestamp;

assert_eq!(
    Timestamp::UNIX_EPOCH.to_string(),
    "1970-01-01T00:00:00Z",
);
```

This is fine most use cases, but it can be useful on occasion to format
a `Timestamp` with a specific offset. While this isn't as expressive
as formatting a datetime with a time zone (e.g., with an IANA time
zone identifier), it may be useful in contexts where you just want to
"hint" at what a user's local time is. To that end, there is a new
`Timestamp::display_with_offset` method that makes this possible:

```rust
use jiff::{tz, Timestamp};

assert_eq!(
    Timestamp::UNIX_EPOCH.display_with_offset(tz::offset(-5)).to_string(),
    "1969-12-31T19:00:00-05:00",
);
```

A corresponding API was added to `jiff::fmt::temporal::DateTimePrinter` for
lower level use.

Moreover, this release also includes new convenience APIs on the Temporal and
RFC 2822 printer types for returning strings. For example, previously, if you
were using the RFC 2822 printer to format a `Timestamp`, you had to do this:

```rust
use jiff::{fmt::rfc2822::DateTimePrinter, Timestamp};

let mut buf = String::new();
DateTimePrinter::new().print_timestamp(&Timestamp::UNIX_EPOCH, &mut buf).unwrap();
assert_eq!(buf, "Thu, 1 Jan 1970 00:00:00 -0000");
```

But now you can just do this:

```rust
use jiff::{fmt::rfc2822::DateTimePrinter, Timestamp};

assert_eq!(
    DateTimePrinter::new().timestamp_to_string(&Timestamp::UNIX_EPOCH).unwrap(),
    "Thu, 1 Jan 1970 00:00:00 -0000",
);
```

Enhancements:

* [#122](https://github.com/BurntSushi/jiff/pull/122):
Support formatting `Timestamp` to an RFC 3339 string with a specific offset.


0.1.11 (2024-08-28)
===================
This release includes a few small enhancements that have been requested over
the last several weeks. The biggest enhancement is a new `jiff::fmt::serde`
sub-module. It provides convenience routines similar to Chrono's
`chrono::serde` sub-module for serializing and deserializing between a
`jiff::Timestamp` and an integer number of seconds, milliseconds, microseconds
or nanoseconds. For example:

```rust
use jiff::Timestamp;

#[derive(serde::Serialize, serde::Deserialize)]
struct Record {
    #[serde(with = "jiff::fmt::serde::timestamp::second::required")]
    timestamp: Timestamp,
}

let json = r#"{"timestamp":1517644800}"#;
let got: Record = serde_json::from_str(&json).unwrap();
assert_eq!(got.timestamp, Timestamp::from_second(1517644800).unwrap());
assert_eq!(serde_json::to_string(&got).unwrap(), json);
```

If you need to support optional timestamps via `Option<Timestamp>`, then use
`jiff::fmt::serde::timestamp::second::optional` instead. Similarly, if you
need to support milliseconds instead of seconds, then replace `second` with
`millisecond` in the module path.

Enhancements:

* [#78](https://github.com/BurntSushi/jiff/issues/78):
Add `BrokenDownTime::set_{offset,iana_time_zone}` APIs.
* [#93](https://github.com/BurntSushi/jiff/issues/93):
Add note about using `Timestamp::now().to_zoned()` instead of
`Zoned::now().with_time_zone()`.
* [#101](https://github.com/BurntSushi/jiff/issues/101):
Add new `jiff::fmt::serde` module for integration with integer timestamps.
* [#117](https://github.com/BurntSushi/jiff/pull/117):
Remove `unsafe` usage in `libm` functions (applicable only to no-std users).


0.1.10 (2024-08-23)
===================
This release features a small bug fix where Jiff will detect an IANA time
zone identifier in some cases where it wouldn't before. While Jiff would
previously read the symlink metadata on `/etc/localtime` by default to discover
the system configured time zone on Unix systems, it *wouldn't* do so when
`TZ=/etc/localtime`. There's really no reason not to, so this release of Jiff
is fixed to use symlink sniffing on file paths provided by thw `TZ` environment
variable.

Bug fixes:

* [#113](https://github.com/BurntSushi/jiff/issues/113):
When `TZ=/etc/localtime`, use symlink metadata to detect IANA identifier.


0.1.9 (2024-08-23)
==================
This release introduces new options for controlling the precision
of fractional seconds when printing `Zoned`, `Timestamp`,
`civil::DateTime` or `civil::Time` values. This is principally exposed
via `jiff::fmt::temporal::DateTimePrinter::precision`, but it's also
available via the standard library's formatting machinery. For example,
if `zdt` is a `jiff::Zoned`, then `format!("{zdt:.6}")` will format
it into a string with microsecond precision, even if its fractional
component is zero.

Enhancements:

* [#92](https://github.com/BurntSushi/jiff/issues/92):
Support setting the precision of fractional seconds when printing datetimes.


0.1.8 (2024-08-19)
==================
This releases fixes a build error in Jiff's `alloc`-only configuration. This
regression was introduced in `jiff 0.1.6`.

Bug fixes:

* [#108](https://github.com/BurntSushi/jiff/issues/108):
Use `core::time::Duration` everywhere instead of `std::time::Duration`.


0.1.7 (2024-08-18)
==================
This release relaxes Jiff's dependency on `windows-sys` to include multiple
semver incompatible releases. The purpose of this relaxation is to enable
Jiff to work with different versions of `windows-sys` in the hopes that this
reduces the likelihood that multiple copies of `windows-sys` are included in
your dependency tree.

Dependencies:

* [#106](https://github.com/BurntSushi/jiff/pull/106):
Relax `windows-sys` dependency constraint to `>=0.52.0, <=0.59.*`.


0.1.6 (2024-08-18)
==================
This release includes a new top-level type, `SignedDuration`, that provides a
near exact replica of `std::time::Duration`, but signed. It is meant to provide
alternative APIs for working with durations at a lower level than what `Span`
provides, and to facilitate better integration with the standard library.

A `SignedDuration` has also been integrated with all of Jiff's datetime types.
For example, previously, `Zoned::checked_add` only accepted a concrete
`jiff::Span`. But now it accepts a `jiff::Span`, `jiff::SignedDuration` or even
a `std::time::Duration`. Moreover, all of the `until` and `since` APIs on
datetime types have been ported and copied to return `SignedDuration` under the
`duration_until` and `duration_since` names.

This marks an initial integration phase with `SignedDuration`. It is planned
to integrate it more with the datetime types. Currently, there are integrations
on `Timestamp` and `Span`, but more will be added in the future.

Overall, folks should still use `Span`. That is the intended default duration
type in Jiff and will continue to be. Users of Jiff may find `SignedDuration`
useful in contexts where speed is important or when one needs to integrate
with the standard library.

This release also includes a few related deprecations as the APIs involving
`std::time::Duration` are phased out in favor of `SignedDuration`.

Deprecations:

* `Timestamp::as_duration`: replaced with `as_jiff_duration`, which will be
renamed to `as_duration` in `jiff 0.2`.
* `Timestamp::from_duration`: replaced with `from_jiff_duration`, which will be
renamed to `from_duration` in `jiff 0.2`.
* `Timestamp::from_signed_duration`: replaced with `from_jiff_duration`.
* `Span::to_duration`: replaced with `to_jiff_duration`, which will be renamed
to `to_duration` in `jiff 0.2`.

Basically, all of the above APIs either accept or return a
`std::time::Duration`. To avoid breaking chnages at this point, new methods
for `SignedDuration` were added. For example, `Timestamp::as_jiff_duration`.
In `jiff 0.2`, the above deprecated methods will be removed and replaced with
equivalent methods that accept or return a `SignedDuration` instead. Callers
can then convert between a `SignedDuration` and a `std::time::Duration` using
appropriate `TryFrom` trait implementations.

Enhancements:

* [#21](https://github.com/BurntSushi/jiff/issues/21):
Add new top-level `SignedDuration` type.
* [#90](https://github.com/BurntSushi/jiff/issues/90):
Improve error message when using `Span` with >=day units with a `Timestamp`.

Performance:

* [#104](https://github.com/BurntSushi/jiff/pull/104)
Optimize offset calculations in time zones without DST.
* [#105](https://github.com/BurntSushi/jiff/pull/105)
Optimize offset calculations for timestamps after last DST rule change.


0.1.5 (2024-08-09)
==================
This release includes some improvements and bug fixes, particularly for Jiff's
`strtime` APIs.

Enhancements:

* [#63](https://github.com/BurntSushi/jiff/issues/63):
Add link to original Chrono maintainer's commentary in `DESIGN.md`.
* [#75](https://github.com/BurntSushi/jiff/issues/75):
Add support for `%V` for formatting _and_ parsing IANA time zone identifiers.
* [#79](https://github.com/BurntSushi/jiff/pull/79):
Add `devcontainer.json` to support GitHub Codespaces.
* [#85](https://github.com/BurntSushi/jiff/pull/85):
Set correct ranges for internal tracking in return value of `days_in_month`.

Bug fixes:

* [#59](https://github.com/BurntSushi/jiff/issues/59):
Fixes a bug where some `Span`s could not be roundtripped through ISO 8601.
* [#71](https://github.com/BurntSushi/jiff/issues/71):
Tweak wording in documentation of "printf"-style API.
* [#73](https://github.com/BurntSushi/jiff/issues/73):
Make it so `%.Nf` only formats to `N` decimal places.
* [#77](https://github.com/BurntSushi/jiff/pull/77):
Disable optimizations when running tests.


0.1.4 (2024-08-01)
==================
This release includes a small improvement for `strptime` that permits
`%Y%m%d` to parse `20240730` correctly.

Enhancements:

* [#62](https://github.com/BurntSushi/jiff/issues/62):
Tweak `strptime` so that things like `%Y` aren't unceremoniously greedy.


0.1.3 (2024-07-30)
==================
This release features support for `wasm32-unknown-unknown`. That is, when
Jiff's new `js` crate feature is enabled, Jiff will automatically use
JavaScript APIs to determine the current time and time zone.

Enhancements:

* [#58](https://github.com/BurntSushi/jiff/pull/58):
Add WASM support and a new `PLATFORM.md` guide.


0.1.2 (2024-07-28)
==================
This release features a few new APIs that a need for arose while experimenting
with actually using Jiff in real projects. Namely, the `jiff::fmt::strtime`
module now has `%f` and `%.f` directives for parsing and formatting fractional
seconds. And both `jiff::fmt::rfc2822` and `jiff::fmt::strtime` now have
support for skipping weekday checks during parsing. (Previously, Jiff required
that an English weekday be consistent with the date parsed, and there was no
way to opt out. While this is still the default behavior, callers can disable
this check.)

Enhancements:

* [#52](https://github.com/BurntSushi/jiff/pull/52):
Improve documentation for `Span` getter methods.
* [#53](https://github.com/BurntSushi/jiff/pull/53):
Add support for skipping weekday checking when parsing datetimes.
* [#55](https://github.com/BurntSushi/jiff/pull/55):
Add support for fractional seconds in `jiff::fmt::strtime`.

Bug fixes:

* [#49](https://github.com/BurntSushi/jiff/pull/49):
Fix informational regex describing ISO 8601 format.
* [#51](https://github.com/BurntSushi/jiff/pull/51):
Explicitly allow new deny-by-default lint `ambiguous_negative_literals`.


0.1.1 (2024-07-25)
==================
This is a new semver compatible release. The principle addition are APIs for
converting between a `jiff::Span` and a `std::time::Duration`. Specifically,
there are now `TryFrom<Span> for Duration` and `TryFrom<Duration> for Span`
trait implementations. There is also a `Span::to_duration`, which requires a
relative date, for converting spans with non-uniform units (like months) to a
`Duration`.

Enhancements:

* [#21](https://github.com/BurntSushi/jiff/issues/21),
  [#40](https://github.com/BurntSushi/jiff/issues/40):
Adds APIs for converting between `Span` and `std::time::Duration`.

Bug fixes:

* [#36](https://github.com/BurntSushi/jiff/issues/36):
Saturating arithmetic for `Timestamp` panics with day-or-greater units.
* [#38](https://github.com/BurntSushi/jiff/issues/38):
Fix some bugs in the micro-benchmarks.
* [#39](https://github.com/BurntSushi/jiff/issues/39):
Document that the RFC 2822 parser is not technically fully spec compliant.


0.1.0 (2024-07-21)
==================
The initial release of Jiff.

[IANA Time Zone Database]: https://www.iana.org/time-zones
[PLATFORM]: PLATFORM.md
[RFC 9110]: https://datatracker.ietf.org/doc/html/rfc9110#section-5.6.7-15
