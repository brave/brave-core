/*!
Contains some tests for the TZif parser in `jiff-core`.

These tests are a little awkward to have live in `jiff-core` because of the use
of `insta` and because I didn't want to duplicate the time zone data at the
time of writing. For the former, I didn't want to use `insta` in `jiff-core`
because I've found it to be somewhat painful to use.
*/

#[cfg(all(test, feature = "alloc"))]
mod tests {
    use alloc::{
        string::{String, ToString},
        vec,
    };

    use jcore::tz::tzif;

    #[cfg(not(miri))]
    use crate::tz::testdata::TZIF_TEST_FILES;

    /// This converts TZif data into a human readable format.
    ///
    /// This is useful for debugging (via `./scripts/jiff-debug tzif`), but we
    /// also use it for snapshot testing to make reading the test output at
    /// least *somewhat* comprehensible for humans. Otherwise, one needs to
    /// read and understand Unix timestamps. That ain't going to fly.
    ///
    /// For this to work, we make sure everything in a `Tzif` value is
    /// represented in some way in this output.
    fn tzif_to_human_readable(tzif: &tzif::TimeZone) -> String {
        use std::io::Write;

        struct IndicatorDisplay(tzif::Indicator);

        impl core::fmt::Display for IndicatorDisplay {
            fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
                match self.0 {
                    tzif::Indicator::LocalWall => write!(f, "local/wall"),
                    tzif::Indicator::LocalStandard => write!(f, "local/std"),
                    tzif::Indicator::UTStandard => write!(f, "ut/std"),
                }
            }
        }

        fn datetime(dt: tzif::DateTime) -> crate::civil::DateTime {
            crate::civil::DateTime::constant(
                dt.year(),
                dt.month(),
                dt.day(),
                dt.hour(),
                dt.minute(),
                dt.second(),
                0,
            )
        }

        let mut out = tabwriter::TabWriter::new(vec![])
            .alignment(tabwriter::Alignment::Left);

        writeln!(out, "TIME ZONE VERSION").unwrap();
        writeln!(out, "  {}", char::try_from(tzif.version).unwrap()).unwrap();

        writeln!(out, "LOCAL TIME TYPES").unwrap();
        for (i, typ) in tzif.types.iter().enumerate() {
            writeln!(
                out,
                "  {i:03}:\toffset={off}\t\
                   designation={desig}\t{dst}\tindicator={ind}",
                off = jcore::tz::Offset::from_seconds(typ.offset.seconds())
                    .unwrap(),
                desig = tzif.designations[usize::from(typ.designation)],
                dst = if typ.dst.is_dst() { "dst" } else { "" },
                ind = IndicatorDisplay(typ.indicator),
            )
            .unwrap();
        }
        if !tzif.transitions.timestamps.is_empty() {
            writeln!(out, "TRANSITIONS").unwrap();
            for i in 0..tzif.transitions.timestamps.len() {
                let trans = &tzif.transitions;
                let timestamp = trans.timestamps[i];
                let dt = jcore::tz::Offset::UTC
                    .to_datetime(timestamp.to_standard_timestamp());
                let typ = tzif.types[usize::from(trans.infos[i].type_index)];
                let wall =
                    alloc::format!("{}", datetime(trans.civil_starts[i]));
                let ambiguous = match trans.infos[i].kind {
                    tzif::TransitionKind::Unambiguous => {
                        "unambiguous".to_string()
                    }
                    tzif::TransitionKind::Gap => {
                        let end = datetime(trans.civil_ends[i]);
                        alloc::format!(" gap-until({end})")
                    }
                    tzif::TransitionKind::Fold => {
                        let end = datetime(trans.civil_ends[i]);
                        alloc::format!("fold-until({end})")
                    }
                };

                writeln!(
                    out,
                    "  {i:04}:\t{dt:?}Z\tunix={ts}\twall={wall}\t\
                       {ambiguous}\t\
                       type={type_index}\t{off}\t\
                       {desig}\t{dst}",
                    ts = timestamp.as_second(),
                    type_index = trans.infos[i].type_index,
                    off =
                        jcore::tz::Offset::from_seconds(typ.offset.seconds())
                            .unwrap(),
                    desig = tzif.designations[usize::from(typ.designation)],
                    dst = if typ.dst.is_dst() { "dst" } else { "" },
                )
                .unwrap();
            }
        }
        if let Some(ref posix_tz) = tzif.posix_tz {
            writeln!(out, "POSIX TIME ZONE STRING").unwrap();
            writeln!(
                out,
                "  {}",
                crate::tz::posix::TimeZoneFormatter(posix_tz)
            )
            .unwrap();
        }
        String::from_utf8(out.into_inner().unwrap()).unwrap()
    }

    /// DEBUG COMMAND
    ///
    /// Takes environment variable `JIFF_DEBUG_TZIF_PATH` as input, and treats
    /// the value as a TZif file path. This test will open the file, parse it
    /// as a TZif and then dump debug data about the file in a human readable
    /// plain text format.
    #[cfg(feature = "std")]
    #[test]
    fn debug_tzif() -> anyhow::Result<()> {
        use anyhow::Context;

        let _ = crate::logging::Logger::init();

        const ENV: &str = "JIFF_DEBUG_TZIF_PATH";
        let Some(val) = std::env::var_os(ENV) else { return Ok(()) };
        let Ok(val) = val.into_string() else {
            anyhow::bail!("{ENV} has invalid UTF-8")
        };
        let bytes =
            std::fs::read(&val).with_context(|| alloc::format!("{val:?}"))?;
        let tzif = tzif::TimeZone::parse(&bytes)?;
        std::eprint!("{}", tzif_to_human_readable(&tzif));
        Ok(())
    }

    #[cfg(not(miri))]
    #[test]
    fn tzif_parse_v2plus() {
        for tzif_test in TZIF_TEST_FILES {
            insta::assert_snapshot!(
                alloc::format!("{}_v2+", tzif_test.name),
                tzif_to_human_readable(&tzif_test.parse())
            );
        }
    }

    #[cfg(not(miri))]
    #[test]
    fn tzif_parse_v1() {
        for tzif_test in TZIF_TEST_FILES {
            insta::assert_snapshot!(
                alloc::format!("{}_v1", tzif_test.name),
                tzif_to_human_readable(&tzif_test.parse_v1())
            );
        }
    }

    /// This tests walks the /usr/share/zoneinfo directory (if it exists) and
    /// tries to parse every TZif formatted file it can find. We don't really
    /// do much with it other than to ensure we don't panic or return an error.
    /// That is, we check that we can parse each file, but not that we do so
    /// correctly.
    #[cfg(not(miri))]
    #[cfg(feature = "tzdb-zoneinfo")]
    #[cfg(target_os = "linux")]
    #[test]
    fn zoneinfo() {
        const TZDIR: &str = "/usr/share/zoneinfo";

        for result in walkdir::WalkDir::new(TZDIR) {
            // Just skip if we got an error traversing the directory tree.
            // These aren't related to our parsing, so it's some other problem
            // (like the directory not existing).
            let Ok(dent) = result else { continue };
            // This test can take some time in debug mode, so skip parsing
            // some of the less frequently used TZif files.
            let Some(name) = dent.path().to_str() else { continue };
            if name.contains("right/") || name.contains("posix/") {
                continue;
            }
            // Again, skip if we can't read. Not my monkeys, not my circus.
            let Ok(bytes) = std::fs::read(dent.path()) else { continue };
            if !tzif::is_possibly_tzif(&bytes) {
                continue;
            }
            // OK at this point, we're pretty sure `bytes` should be a TZif
            // binary file. So try to parse it and fail the test if it fails.
            if let Err(err) = tzif::TimeZone::parse(&bytes) {
                panic!("failed to parse TZif file {:?}: {err}", dent.path());
            }
        }
    }
}
