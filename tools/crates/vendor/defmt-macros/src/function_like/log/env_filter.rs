use std::{
    collections::{BTreeMap, BTreeSet},
    env,
};

use defmt_parser::Level;
use proc_macro2::{Span, TokenStream as TokenStream2};
use quote::quote;

use self::parse::{Entry, LogLevelOrOff, ModulePath};

mod parse;

#[derive(Debug)]
pub(crate) struct EnvFilter {
    // to keep the module paths sorted by length we use a btreemap
    entries: BTreeMap<ModulePath, LogLevelOrOff>,
}

impl EnvFilter {
    pub(crate) fn from_env_var() -> syn::Result<Self> {
        let defmt_log = env::var("DEFMT_LOG").ok();
        let Ok(cargo_crate_name) = env::var("CARGO_CRATE_NAME") else {
            return Err(syn::Error::new(
                Span::call_site(),
                "`CARGO_CRATE_NAME` env var is not set",
            ));
        };
        Self::new(defmt_log.as_deref(), &cargo_crate_name)
    }

    fn new(defmt_log: Option<&str>, cargo_crate_name: &str) -> syn::Result<Self> {
        // match `env_logger` behavior
        const LEVEL_WHEN_LEVEL_IS_NOT_SPECIFIED: LogLevelOrOff = Some(Level::Trace);
        const LEVEL_WHEN_NOTHING_IS_SPECIFIED: LogLevelOrOff = Some(Level::Error);

        let caller_crate = cargo_crate_name;

        let mut entries = BTreeMap::new();
        let mut fallback_log_level = None;
        if let Some(input) = defmt_log {
            for entry in parse::defmt_log(input) {
                let (modpath, level) = match entry? {
                    Entry::LogLevel(log_level) => {
                        if fallback_log_level.is_none() {
                            fallback_log_level = Some(log_level);
                        }
                        continue;
                    }
                    Entry::ModulePath(module) => (module, LEVEL_WHEN_LEVEL_IS_NOT_SPECIFIED),
                    Entry::ModulePathLogLevel {
                        module_path,
                        log_level,
                    } => (module_path, log_level),
                };

                if modpath.crate_name() == caller_crate && !entries.contains_key(&modpath) {
                    entries.insert(modpath, level);
                }
            }
        }

        let modpath = ModulePath::from_crate_name(caller_crate);
        entries
            .entry(modpath)
            .or_insert_with(|| fallback_log_level.unwrap_or(LEVEL_WHEN_NOTHING_IS_SPECIFIED));

        Ok(EnvFilter { entries })
    }

    /// Builds a compile-time check that returns `true` when `module_path!` can emit logs at the
    /// requested log `level`
    ///
    /// Returns `None` if the caller crate (at any module path) will never emit logs at requested log `level`
    pub(crate) fn path_check(&self, level: Level) -> Option<TokenStream2> {
        enum Criteria {
            Accept,
            Reject,
        }

        let modules_to_accept = self.modules_on_for(level);

        if modules_to_accept.is_empty() {
            return None;
        }

        let modules_to_reject = self.modules_off_for(level);

        let module_to_criteria: BTreeMap<_, _> = modules_to_accept
            .iter()
            .map(|&path| (path, Criteria::Accept))
            .chain(
                modules_to_reject
                    .iter()
                    .map(|&path| (path, Criteria::Reject)),
            )
            .collect();

        // iterate in reverse because we want to early accept innermost modules
        // the iteration will go `krate::module::inner`, then `krate::module` then `krate`
        let checks = module_to_criteria
            .iter()
            .rev()
            .map(|(&module_path, criteria)| {
                let check = codegen_is_inside_of_check(&module_path.to_string());
                let retval = match criteria {
                    Criteria::Accept => quote!(true),
                    Criteria::Reject => quote!(false),
                };
                quote!(if #check {
                    return #retval;
                })
            })
            .collect::<Vec<_>>();

        Some(quote!({
            const CHECK: bool = {
                const fn check() -> bool {
                    let module_path = module_path!().as_bytes();
                    #(#checks)*
                    false
                }
                check()
            };
            CHECK
        }))
    }

    /// Returns the set of modules that enable logs at requested `level`
    fn modules_on_for(&self, level: Level) -> BTreeSet<&ModulePath> {
        self.entries
            .iter()
            .filter_map(|(module_path, min_level)| {
                // `min_level == None` means "off" so exclude the module path in that case
                min_level.and_then(|min_level| {
                    if level >= min_level {
                        Some(module_path)
                    } else {
                        None
                    }
                })
            })
            .collect()
    }

    /// Returns the set of modules that disable logs at requested `level`
    fn modules_off_for(&self, level: Level) -> BTreeSet<&ModulePath> {
        self.entries
            .iter()
            .filter_map(|(module_path, min_level)| match min_level {
                Some(min_level) if level >= *min_level => None,
                _ => {
                    // The module is either `off` or has a higher minimum level.
                    Some(module_path)
                }
            })
            .collect()
    }
}

// NOTE this also returns `true` when `module_path == parent_module_path`
// what we want to check is if the function that calls the proc-macro is inside `parent_module_path`
// `module_path!` returns the path to the module the function is in, not the path to the function
// itself
fn codegen_is_inside_of_check(parent_module_path: &str) -> TokenStream2 {
    let parent = parent_module_path.as_bytes();
    let parent_len = parent.len();
    let byte_checks = parent
        .iter()
        .enumerate()
        .map(|(index, byte)| quote!(module_path[#index] == #byte))
        .collect::<Vec<_>>();

    quote!(
        // start of const-context `[u8]::starts_with(needle)`
        if #parent_len > module_path.len() {
            false
        } else {
            #(#byte_checks &&)*
        // end of const-context `[u8]::starts_with`

        // check that what follows the `module_path` is the end of a path segment
        if #parent_len == module_path.len() {
            // end of the entire module path
            true
        } else {
            // end of module path _segment_
            //
            // `module_path` comes from `module_path!`; we assume it's well-formed so we
            // don't check *everything* that comes after `needle`; just the first
            // character of what should be the path separator ("::")
            module_path[#parent_len] == b':'
        }
    })
}

#[cfg(test)]
mod tests {
    use maplit::btreeset;
    use pretty_assertions::assert_eq;

    use super::*;

    #[test]
    fn when_duplicates_entries_in_defmt_log_use_last_entry() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate=info,krate=debug"), "krate")?;
        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Debug)
        );
        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Trace));
        Ok(())
    }

    #[test]
    fn when_empty_defmt_log_use_error() -> syn::Result<()> {
        let env_filter = EnvFilter::new(None, "krate")?;
        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Error)
        );
        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Warn));
        Ok(())
    }

    #[test]
    fn when_no_level_in_defmt_log_use_trace() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate"), "krate")?;
        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Trace)
        );
        Ok(())
    }

    #[test]
    fn when_level_in_defmt_log_use_it() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate=info"), "krate")?;
        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Info)
        );
        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Debug));
        Ok(())
    }

    #[test]
    fn when_only_level_is_specified_in_defmt_log_it_applies_to_all_crates() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("info"), "krate")?;
        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Info)
        );
        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Debug));
        Ok(())
    }

    #[test]
    fn moduleless_level_has_lower_precedence() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate=info,warn"), "krate")?;
        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Info)
        );
        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Debug));
        Ok(())
    }

    #[test]
    fn moduleless_level_behaves_like_a_krate_level_pair() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate::module=info,warn"), "krate")?;
        let expected = [
            ModulePath::parse("krate"),
            ModulePath::parse("krate::module"),
        ];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Warn)
        );

        let expected = [ModulePath::parse("krate::module")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Info)
        );

        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Debug));
        Ok(())
    }

    #[test]
    fn module_paths_different_levels() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate=info,krate::module=debug"), "krate")?;

        let expected = [
            ModulePath::parse("krate"),
            ModulePath::parse("krate::module"),
        ];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Info)
        );

        let expected = [ModulePath::parse("krate::module")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Debug)
        );

        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Trace));
        Ok(())
    }

    #[test]
    fn blanket_off() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("off"), "krate")?;

        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Error));

        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_off_for(Level::Error)
        );
        Ok(())
    }

    #[test]
    fn blanket_off_plus_override() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate::module=error,off"), "krate")?;

        let expected = [ModulePath::parse("krate::module")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Error)
        );

        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Warn));

        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_off_for(Level::Error)
        );
        Ok(())
    }

    #[test]
    fn child_can_be_more_restrictive_than_parent() -> syn::Result<()> {
        let env_filter = EnvFilter::new(
            Some("debug,krate::parent=trace,krate::parent::child=debug"),
            "krate",
        )?;

        let expected_on = [ModulePath::parse("krate::parent")];
        assert_eq!(
            expected_on.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Trace)
        );

        let expected_off = [
            ModulePath::parse("krate"),
            ModulePath::parse("krate::parent::child"),
        ];
        assert_eq!(
            expected_off.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_off_for(Level::Trace)
        );
        Ok(())
    }

    #[test]
    fn does_not_match_partial_crate_name() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("fooo=warn"), "foo")?;
        let expected = [ModulePath::parse("foo")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Error)
        );
        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Warn));
        Ok(())
    }

    // doesn't affect runtime performance but it makes the expanded code smaller
    #[ignore = "TODO(P-low/optimization): impl & more test cases"]
    #[test]
    fn when_module_paths_with_same_level_remove_inner_ones() -> syn::Result<()> {
        let env_filter = EnvFilter::new(Some("krate=info,krate::module=info"), "krate")?;
        let expected = [ModulePath::parse("krate")];
        assert_eq!(
            expected.iter().collect::<BTreeSet<_>>(),
            env_filter.modules_on_for(Level::Info)
        );
        assert_eq!(btreeset![], env_filter.modules_on_for(Level::Debug));
        Ok(())
    }
}
