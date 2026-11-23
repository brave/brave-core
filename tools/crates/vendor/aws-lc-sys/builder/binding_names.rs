// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

pub(crate) fn strip_binding_link_prefixes(bindings: &str) -> String {
    // Bindgen emits LLVM's \u{1} escape to suppress symbol mangling, which
    // Cranelift does not support. Drop it (and any explicit platform underscore)
    // so the backend applies platform mangling itself. Only AWS-LC symbols are
    // rewritten; unrelated asm aliases may need verbatim names.
    //
    // Text rewrite rather than `ParseCallbacks`: bindgen adds the escape after
    // all callbacks run, and `bindgen-cli` has no hook. Matching from '[' covers
    // both rustfmt's `#[...]` and the unformatted `# [...]`.
    bindings
        .replace(r#"[link_name = "\u{1}_aws_lc_"#, r#"[link_name = "aws_lc_"#)
        .replace(r#"[link_name = "\u{1}aws_lc_"#, r#"[link_name = "aws_lc_"#)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_strip_binding_link_prefixes() {
        for library_prefix in ["aws_lc_0_45_0", "aws_lc_fips_0_14_2"] {
            for platform_prefix in [r"\u{1}", r"\u{1}_", ""] {
                let bindings = format!(
                    "#[link_name = \"{platform_prefix}{library_prefix}_SHA256\"]\n\
                     pub fn SHA256();\n\
                     #[link_name = \"{platform_prefix}{library_prefix}_OPENSSL_ia32cap_P\"]\n\
                     pub static mut OPENSSL_ia32cap_P: [u32; 4];\n"
                );
                let expected = format!(
                    "#[link_name = \"{library_prefix}_SHA256\"]\n\
                     pub fn SHA256();\n\
                     #[link_name = \"{library_prefix}_OPENSSL_ia32cap_P\"]\n\
                     pub static mut OPENSSL_ia32cap_P: [u32; 4];\n"
                );
                let normalized = strip_binding_link_prefixes(&bindings);
                assert_eq!(normalized, expected);
                assert_eq!(strip_binding_link_prefixes(&normalized), normalized);
            }
        }
    }

    #[test]
    fn test_strip_binding_link_prefixes_unformatted() {
        for library_prefix in ["aws_lc_0_45_0", "aws_lc_fips_0_14_2"] {
            for platform_prefix in [r"\u{1}", r"\u{1}_", ""] {
                // TokenStream::to_string() is bindgen's fallback when rustfmt fails.
                let bindings = format!(
                    r#"extern "C" {{ # [link_name = "{platform_prefix}{library_prefix}_SHA256"] pub fn SHA256 () ; # [link_name = "{platform_prefix}{library_prefix}_OPENSSL_ia32cap_P"] pub static mut OPENSSL_ia32cap_P : [u32 ; 4] ; }}"#
                );
                let expected = format!(
                    r#"extern "C" {{ # [link_name = "{library_prefix}_SHA256"] pub fn SHA256 () ; # [link_name = "{library_prefix}_OPENSSL_ia32cap_P"] pub static mut OPENSSL_ia32cap_P : [u32 ; 4] ; }}"#
                );
                let normalized = strip_binding_link_prefixes(&bindings);
                assert_eq!(normalized, expected);
                assert_eq!(strip_binding_link_prefixes(&normalized), normalized);
            }
        }
    }

    #[test]
    fn test_strip_binding_link_prefixes_preserves_other_content() {
        let bindings = r#"pub const TEXT: &str = "\u{1}_aws_lc_";
#[link_name = "\u{1}unrelated_symbol"]
pub fn unrelated_symbol();
#[link_name = "_already_mangled_symbol"]
pub fn already_mangled_symbol();
# [link_name = "\u{1}_unrelated_symbol"] pub fn unformatted_unrelated_symbol () ;
extern "C" {
    pub fn SHA256();
}
"#;
        assert_eq!(strip_binding_link_prefixes(bindings), bindings);
        assert_eq!(strip_binding_link_prefixes(""), "");
    }
}
