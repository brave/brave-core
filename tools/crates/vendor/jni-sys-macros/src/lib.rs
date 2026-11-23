extern crate proc_macro;

use std::{cmp::Ordering, collections::HashSet};

use proc_macro::TokenStream;
use quote::{format_ident, quote};
use syn::{parse_macro_input, spanned::Spanned, Data, DeriveInput, Fields, Ident, LitStr};

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
struct JniVersion {
    major: u16,
    minor: u16,
}
impl Default for JniVersion {
    fn default() -> Self {
        Self { major: 1, minor: 1 }
    }
}
impl Ord for JniVersion {
    fn cmp(&self, other: &Self) -> Ordering {
        match self.major.cmp(&other.major) {
            Ordering::Equal => self.minor.cmp(&other.minor),
            major_order => major_order,
        }
    }
}
impl PartialOrd for JniVersion {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl syn::parse::Parse for JniVersion {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Self> {
        let version: LitStr = input.parse()?;
        let version = version.value();
        if version == "reserved" {
            // We special case version 999 later instead of making JniVersion an enum
            return Ok(JniVersion {
                major: 999,
                minor: 0,
            });
        }
        let mut split = version.splitn(2, '.');
        const EXPECTED_MSG: &str = "Expected \"major.minor\" version number or \"reserved\"";
        let major = split
            .next()
            .ok_or(syn::Error::new(input.span(), EXPECTED_MSG))?;
        let major = major
            .parse::<u16>()
            .map_err(|_| syn::Error::new(input.span(), EXPECTED_MSG))?;
        let minor = split
            .next()
            .unwrap_or("0")
            .parse::<u16>()
            .map_err(|_| syn::Error::new(input.span(), EXPECTED_MSG))?;
        Ok(JniVersion { major, minor })
    }
}

fn jni_to_union_impl(input: DeriveInput) -> syn::Result<TokenStream> {
    let original_name = &input.ident;
    let original_visibility = &input.vis;

    let mut versions = HashSet::new();
    let mut versioned_fields = vec![];

    if let Data::Struct(data) = &input.data {
        if let Fields::Named(fields) = &data.fields {
            for field in &fields.named {
                // Default to version 1.1
                let mut min_version = JniVersion::default();

                let mut field = field.clone();

                let mut jni_added_attr = None;
                field.attrs.retain(|attr| {
                    if attr.path().is_ident("jni_added") {
                        jni_added_attr = Some(attr.clone());
                        false
                    } else {
                        true
                    }
                });
                if let Some(attr) = jni_added_attr {
                    let version = attr.parse_args::<JniVersion>()?;
                    min_version = version;
                }

                versions.insert(min_version);
                versioned_fields.push((min_version, field.clone()));
            }

            // Quote structs and union
            let mut expanded = quote! {};

            let mut union_members = quote!();

            let mut versions: Vec<_> = versions.into_iter().collect();
            versions.sort();

            for version in versions {
                let (struct_ident, version_ident, version_suffix) = if version.major == 999 {
                    (
                        Ident::new(&format!("{}_reserved", original_name), original_name.span()),
                        Ident::new("reserved", original_name.span()),
                        "reserved".to_string(),
                    )
                } else if version.minor == 0 {
                    (
                        Ident::new(
                            &format!("{}_{}", original_name, version.major),
                            original_name.span(),
                        ),
                        Ident::new(&format!("v{}", version.major), original_name.span()),
                        format!("{}", version.major),
                    )
                } else {
                    let struct_ident = Ident::new(
                        &format!("{}_{}_{}", original_name, version.major, version.minor),
                        original_name.span(),
                    );
                    let version_ident = Ident::new(
                        &format!("v{}_{}", version.major, version.minor),
                        original_name.span(),
                    );
                    (
                        struct_ident,
                        version_ident,
                        format!("{}_{}", version.major, version.minor),
                    )
                };

                let last = versioned_fields
                    .iter()
                    .rposition(|(v, _f)| v <= &version)
                    .unwrap_or(versioned_fields.len());
                let mut padding_idx = 0u32;

                let mut version_field_tokens = quote!();
                for (i, (field_min_version, field)) in versioned_fields.iter().enumerate() {
                    if i > last {
                        break;
                    }
                    if field_min_version > &version {
                        let reserved_ident = format_ident!("_padding_{}", padding_idx);
                        padding_idx += 1;
                        version_field_tokens.extend(quote! { #reserved_ident: *mut c_void, });
                    } else {
                        version_field_tokens.extend(quote! { #field, });
                    }
                }
                expanded.extend(quote! {
                    #[allow(non_snake_case, non_camel_case_types)]
                    #[repr(C)]
                    #[derive(Copy, Clone)]
                    #original_visibility struct #struct_ident {
                        #version_field_tokens
                    }
                });

                let api_comment =
                    format!("API when JNI version >= `JNI_VERSION_{}`", version_suffix);
                union_members.extend(quote! {
                    #[doc = #api_comment]
                    #original_visibility #version_ident: #struct_ident,
                });
            }

            expanded.extend(quote! {
                #[repr(C)]
                #original_visibility union #original_name {
                    #union_members
                }
            });

            return Ok(TokenStream::from(expanded));
        }
    }

    Err(syn::Error::new(
        input.span(),
        "Expected a struct with fields",
    ))
}

#[proc_macro_attribute]
pub fn jni_to_union(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let input = parse_macro_input!(item as DeriveInput);

    match jni_to_union_impl(input) {
        Ok(tokens) => tokens,
        Err(err) => err.into_compile_error().into(),
    }
}
