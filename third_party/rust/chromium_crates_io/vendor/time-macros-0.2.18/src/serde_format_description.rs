use proc_macro::{Ident, TokenStream, TokenTree};

pub(crate) fn build(
    mod_name: Ident,
    ty: TokenTree,
    format: TokenStream,
    format_description_display: String,
) -> TokenStream {
    let ty_s = &*ty.to_string();

    let visitor = if cfg!(feature = "parsing") {
        quote! {
            struct Visitor;
            struct OptionVisitor;

            impl<'a> ::serde::de::Visitor<'a> for Visitor {
                type Value = __TimeSerdeType;

                fn expecting(&self, f: &mut ::std::fmt::Formatter<'_>) -> ::std::fmt::Result {
                    write!(
                        f,
                        concat!(
                            "a(n) `",
                            #(ty_s),
                            "` in the format \"{}\"",
                        ),
                        #(format_description_display.as_str())
                    )
                }

                fn visit_str<E: ::serde::de::Error>(
                    self,
                    value: &str
                ) -> Result<__TimeSerdeType, E> {
                    __TimeSerdeType::parse(value, &description()).map_err(E::custom)
                }
            }

            impl<'a> ::serde::de::Visitor<'a> for OptionVisitor {
                type Value = Option<__TimeSerdeType>;

                fn expecting(&self, f: &mut ::std::fmt::Formatter<'_>) -> ::std::fmt::Result {
                    write!(
                        f,
                        concat!(
                            "an `Option<",
                            #(ty_s),
                            ">` in the format \"{}\"",
                        ),
                        #(format_description_display.as_str())
                    )
                }

                fn visit_some<D: ::serde::de::Deserializer<'a>>(
                    self,
                    deserializer: D
                ) -> Result<Option<__TimeSerdeType>, D::Error> {
                    deserializer
                        .deserialize_str(Visitor)
                        .map(Some)
                }

                fn visit_none<E: ::serde::de::Error>(
                    self
                ) -> Result<Option<__TimeSerdeType>, E> {
                    Ok(None)
                }
            }
        }
    } else {
        quote!()
    };

    let serialize_primary = if cfg!(feature = "formatting") {
        quote! {
            pub fn serialize<S: ::serde::Serializer>(
                datetime: &__TimeSerdeType,
                serializer: S,
            ) -> Result<S::Ok, S::Error> {
                use ::serde::Serialize;
                datetime
                    .format(&description())
                    .map_err(::time::error::Format::into_invalid_serde_value::<S>)?
                    .serialize(serializer)
            }
        }
    } else {
        quote!()
    };

    let deserialize_primary = if cfg!(feature = "parsing") {
        quote! {
            pub fn deserialize<'a, D: ::serde::Deserializer<'a>>(
                deserializer: D
            ) -> Result<__TimeSerdeType, D::Error> {
                use ::serde::Deserialize;
                deserializer.deserialize_str(Visitor)
            }
        }
    } else {
        quote!()
    };

    let serialize_option = if cfg!(feature = "formatting") {
        quote! {
            pub fn serialize<S: ::serde::Serializer>(
                option: &Option<__TimeSerdeType>,
                serializer: S,
            ) -> Result<S::Ok, S::Error> {
                use ::serde::Serialize;
                option.map(|datetime| datetime.format(&description()))
                    .transpose()
                    .map_err(::time::error::Format::into_invalid_serde_value::<S>)?
                    .serialize(serializer)
            }
        }
    } else {
        quote!()
    };

    let deserialize_option = if cfg!(feature = "parsing") {
        quote! {
            pub fn deserialize<'a, D: ::serde::Deserializer<'a>>(
                deserializer: D
            ) -> Result<Option<__TimeSerdeType>, D::Error> {
                use ::serde::Deserialize;
                deserializer.deserialize_option(OptionVisitor)
            }
        }
    } else {
        quote!()
    };

    let deserialize_option_imports = if cfg!(feature = "parsing") {
        quote! {
            use super::{OptionVisitor, Visitor};
        }
    } else {
        quote!()
    };

    let fd_traits = match (cfg!(feature = "formatting"), cfg!(feature = "parsing")) {
        (false, false) => {
            bug!("serde_format_description::build called without formatting or parsing enabled")
        }
        (false, true) => quote! { ::time::parsing::Parsable },
        (true, false) => quote! { ::time::formatting::Formattable },
        (true, true) => quote! { ::time::formatting::Formattable + ::time::parsing::Parsable },
    };

    quote! {
        mod #(mod_name) {
            use ::time::#(ty) as __TimeSerdeType;

            const fn description() -> impl #S(fd_traits) {
                #S(format)
            }

            #S(visitor)
            #S(serialize_primary)
            #S(deserialize_primary)

            pub(super) mod option {
                use super::{description, __TimeSerdeType};
                #S(deserialize_option_imports)

                #S(serialize_option)
                #S(deserialize_option)
            }
        }
    }
}
