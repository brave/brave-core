use defmt_parser::Level;
use proc_macro2::Span;
use std::fmt;
use syn::Ident;

// None = "off" pseudo-level
pub(crate) type LogLevelOrOff = Option<Level>;

// NOTE this is simpler than `syn::Path`; we do not want to accept e.g. `Vec::<Ty>::new`
#[derive(Debug, Eq, Ord, PartialEq, PartialOrd)]
pub(crate) struct ModulePath {
    segments: Vec<String>,
}

/// Parses the contents of the `DEFMT_LOG` env var
pub(crate) fn defmt_log(input: &str) -> impl Iterator<Item = syn::Result<Entry>> + '_ {
    input
        .rsplit(',')
        .filter(|entry| !entry.is_empty())
        .map(|entry| {
            if let Some((path, log_level)) = entry.rsplit_once('=') {
                let module_path = ModulePath::parse(path);
                let Ok(log_level) = parse_log_level(log_level) else {
                    return Err(syn::Error::new(
                        Span::call_site(),
                        format!(
                            "unknown log level `{}` in DEFMT_LOG env var. \
                            expected one of: off, error, info, warn, debug, trace",
                            log_level
                        ),
                    ));
                };

                Ok(Entry::ModulePathLogLevel {
                    module_path,
                    log_level,
                })
            } else if let Ok(log_level) = parse_log_level(entry) {
                Ok(Entry::LogLevel(log_level))
            } else {
                Ok(Entry::ModulePath(ModulePath::parse(entry)))
            }
        })
}

#[derive(Debug, PartialEq)]
pub(crate) enum Entry {
    LogLevel(LogLevelOrOff),
    ModulePath(ModulePath),
    ModulePathLogLevel {
        module_path: ModulePath,
        log_level: LogLevelOrOff,
    },
}

impl ModulePath {
    pub(crate) fn from_crate_name(input: &str) -> Self {
        if input.is_empty() || input.contains("::") {
            panic!(
                "DEFMT_LOG env var: crate name cannot be an empty string or contain path separators"
            )
        }
        Self::parse(input)
    }

    pub(super) fn parse(input: &str) -> Self {
        if input.is_empty() {
            panic!("DEFMT_LOG env var: module path cannot be an empty string")
        }

        input.split("::").for_each(validate_identifier);

        Self {
            segments: input
                .split("::")
                .map(|segment| segment.to_string())
                .collect(),
        }
    }

    pub(super) fn crate_name(&self) -> &str {
        &self.segments[0]
    }
}

impl fmt::Display for ModulePath {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.segments.join("::"))
    }
}

fn parse_log_level(input: &str) -> Result<LogLevelOrOff, ()> {
    Ok(Some(match input {
        "debug" => Level::Debug,
        "error" => Level::Error,
        "info" => Level::Info,
        "off" => return Ok(None),
        "trace" => Level::Trace,
        "warn" => Level::Warn,
        _ => return Err(()),
    }))
}

fn validate_identifier(input: &str) {
    syn::parse_str::<Ident>(input)
        .unwrap_or_else(|_| panic!("`{input}` is not a valid identifier"));
}

#[cfg(test)]
mod tests {
    use pretty_assertions::assert_eq;
    use rstest::rstest;

    use super::*;

    #[test]
    fn parses_from_the_right() -> syn::Result<()> {
        let entries = defmt_log("krate=info,krate,info").collect::<syn::Result<Vec<_>>>()?;
        assert_eq!(
            [
                Entry::LogLevel(Some(Level::Info)),
                Entry::ModulePath(ModulePath {
                    segments: vec!["krate".to_string()]
                }),
                Entry::ModulePathLogLevel {
                    module_path: ModulePath {
                        segments: vec!["krate".to_string()]
                    },
                    log_level: Some(Level::Info)
                },
            ],
            entries.as_slice()
        );
        Ok(())
    }

    #[test]
    fn after_sorting_innermost_modules_appear_last() {
        let mut paths = [
            ModulePath::parse("krate::module::inner"),
            ModulePath::parse("krate"),
            ModulePath::parse("krate::module"),
        ];
        paths.sort();

        let expected = [
            ModulePath::parse("krate"),
            ModulePath::parse("krate::module"),
            ModulePath::parse("krate::module::inner"),
        ];
        assert_eq!(expected, paths);
    }

    #[test]
    fn accepts_raw_identifier() {
        ModulePath::parse("krate::r#mod");
    }

    #[rstest]
    #[case::has_module("krate::module")]
    #[case::no_module("krate")]
    fn modpath_crate_name(#[case] input: &str) {
        let modpath = ModulePath::parse(input);
        assert_eq!("krate", modpath.crate_name());
    }

    #[rstest]
    #[case::crate_name_is_invalid("some-crate::module")]
    #[case::module_name_is_invalid("krate::some-module")]
    #[case::with_level("krate::some-module=info")]
    #[should_panic = "not a valid identifier"]
    fn rejects_invalid_identifier(#[case] input: &str) {
        defmt_log(input).collect::<syn::Result<Vec<_>>>().unwrap();
    }

    #[test]
    #[should_panic = "unknown log level"]
    fn rejects_unknown_log_level() {
        defmt_log("krate=module")
            .collect::<syn::Result<Vec<_>>>()
            .unwrap();
    }

    #[test]
    #[should_panic = "module path cannot be an empty string"]
    fn rejects_empty_module_path() {
        defmt_log("=info").collect::<syn::Result<Vec<_>>>().unwrap();
    }
}
