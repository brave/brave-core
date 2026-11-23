use std::collections::HashSet;

use cargo_lock::Package;
use object::{File, Object, ObjectSymbol};
use rustc_demangle::demangle;
use rustsec::advisory::affected::FunctionPath;
use syn::{Ident, Type, TypePath, parse_str};

pub(crate) struct SymbolSet(Vec<Vec<Ident>>);

impl SymbolSet {
    /// Extract and demangle all symbols from a binary.
    pub(crate) fn from_file<'a>(
        contents: &[u8],
        vulnerable_crates: impl Iterator<Item = &'a Package>,
    ) -> Result<Self, object::read::Error> {
        let crate_names = vulnerable_crates
            .map(|c| c.name.as_str().replace('-', "_"))
            .collect::<HashSet<_>>();

        let file = File::parse(contents)?;
        let mut symbols = Vec::new();
        for symbol in file.symbols() {
            let Ok(name) = symbol.name() else {
                continue;
            };

            // `parse_str::<TypePath>` is expensive. The filter on `crate_names`
            // eliminates symbols that we know would be irrelevant.
            if !crate_names
                .iter()
                .any(|crate_name| name.contains(crate_name.as_str()))
            {
                continue;
            }

            let name = format!("{:#}", demangle(name));
            if let Ok(type_path) = parse_str::<TypePath>(&name) {
                symbols.push(flatten_type_path(&type_path));
            }
        }

        symbols.sort();
        symbols.dedup();
        Ok(Self(symbols))
    }

    pub(crate) fn filter(
        &self,
        affected: impl IntoIterator<Item = FunctionPath>,
    ) -> impl Iterator<Item = FunctionPath> {
        affected.into_iter().filter(|affected| {
            let affected = affected
                .iter()
                .map(|ident| match ident.as_str().split_once('<') {
                    Some((path, _)) => path,
                    None => ident.as_str(),
                })
                .collect::<Vec<_>>();

            self.0.iter().any(|symbol| {
                match (symbol.as_slice(), affected.as_slice()) {
                    ([ident], [affected]) => ident == affected,
                    (
                        [ident_first, ident_middle @ .., ident_last],
                        [affected_first, affected_middle @ .., affected_last],
                    ) => {
                        // First segments must match (crate name).
                        ident_first == affected_first
                            // In between the first and last segments, the function path segments must
                            // be a subsequence of the symbol segments.
                            && is_subsequence(affected_middle, ident_middle)
                            // Last segments must match (function name).
                            && ident_last == affected_last
                    }
                    (_, _) => false,
                }
            })
        })
    }
}

fn flatten_type_path(mut type_path: &TypePath) -> Vec<Ident> {
    let mut idents = Vec::new();
    let mut stack = Vec::new();
    loop {
        stack.push(type_path);
        if let Some(qself) = &type_path.qself
            && let Type::Path(inner) = &*qself.ty
        {
            type_path = inner;
            continue;
        } else {
            break;
        }
    }

    while let Some(type_path) = stack.pop() {
        for segment in &type_path.path.segments {
            // Discard any generic parameters.
            idents.push(segment.ident.clone());
        }
    }

    idents
}

fn is_subsequence(function_path: &[&str], symbol: &[Ident]) -> bool {
    let mut symbol_iter = symbol.iter();
    for function_path_seg in function_path {
        loop {
            match symbol_iter.next() {
                Some(symbol_seg) if symbol_seg == function_path_seg => break,
                Some(_) => {}
                None => return false,
            }
        }
    }
    true
}

#[cfg(test)]
mod tests {
    use std::{str::FromStr, vec};

    use cargo_lock::Name;
    use rustsec::Version;

    use super::*;

    // Test case based on https://rustsec.org/advisories/RUSTSEC-2024-0360
    #[test]
    fn filter() {
        let package = Package {
            name: Name::from_str("xmp_toolkit").unwrap(),
            version: Version::from_str("0.1.0").unwrap(),
            source: None,
            replace: None,
            checksum: None,
            dependencies: Vec::new(),
        };

        let set = SymbolSet::from_file(
            include_bytes!("../tests/support/binaries/binary-with-affected-functions"),
            [&package].into_iter(),
        )
        .unwrap();

        let affected = vec![FunctionPath::from_str("xmp_toolkit::XmpFile::close").unwrap()];
        let filtered = set.filter(affected).collect::<Vec<_>>();
        assert_eq!(
            filtered,
            vec![FunctionPath::from_str("xmp_toolkit::XmpFile::close").unwrap()]
        );
    }
}
