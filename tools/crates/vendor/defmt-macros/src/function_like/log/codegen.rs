use defmt_parser::{Fragment, Parameter, Type};
use proc_macro2::{Ident as Ident2, Span as Span2, TokenStream as TokenStream2};
use quote::{format_ident, quote};

pub(crate) struct Codegen {
    pub(crate) exprs: Vec<TokenStream2>,
    pub(crate) patterns: Vec<Ident2>,
}

impl Codegen {
    pub(crate) fn new(
        fragments: &[Fragment<'_>],
        given_arg_count: usize,
        span: Span2,
    ) -> syn::Result<Self> {
        let params = fragments
            .iter()
            .filter_map(|frag| match frag {
                Fragment::Parameter(param) => Some(param.clone()),
                Fragment::Literal(_) => None,
            })
            .collect::<Vec<_>>();

        let expected_arg_count = params
            .iter()
            .map(|param| param.index + 1)
            .max()
            .unwrap_or(0);

        if given_arg_count != expected_arg_count {
            let mut only = "";
            if given_arg_count < expected_arg_count {
                only = "only ";
            }

            return Err(syn::Error::new(
                span,
                format!(
                    "format string requires {} arguments but {}{} were provided",
                    expected_arg_count, only, given_arg_count
                ),
            ));
        }

        let mut exprs = vec![];
        let mut patterns = vec![];

        for arg_index in 0..expected_arg_count {
            let arg_ident = format_ident!("arg{}", arg_index);
            let matching_param = params
                .iter()
                .find(|param| param.index == arg_index)
                .unwrap();

            let expr = encode_arg(&matching_param.ty, &params, arg_index, &arg_ident);

            exprs.push(expr);
            patterns.push(arg_ident);
        }

        Ok(Codegen { exprs, patterns })
    }
}

fn encode_arg(ty: &Type, params: &[Parameter], arg_index: usize, arg: &Ident2) -> TokenStream2 {
    match ty {
        Type::I8 => quote!(defmt::export::i8(#arg)),
        Type::I16 => quote!(defmt::export::i16(#arg)),
        Type::I32 => quote!(defmt::export::i32(#arg)),
        Type::I64 => quote!(defmt::export::i64(#arg)),
        Type::I128 => quote!(defmt::export::i128(#arg)),
        Type::Isize => quote!(defmt::export::isize(#arg)),

        Type::U8 => quote!(defmt::export::u8(#arg)),
        Type::U16 => quote!(defmt::export::u16(#arg)),
        Type::U32 => quote!(defmt::export::u32(#arg)),
        Type::U64 => quote!(defmt::export::u64(#arg)),
        Type::U128 => quote!(defmt::export::u128(#arg)),
        Type::Usize => quote!(defmt::export::usize(#arg)),

        Type::F32 => quote!(defmt::export::f32(#arg)),
        Type::F64 => quote!(defmt::export::f64(#arg)),

        Type::Bool => quote!(defmt::export::bool(#arg)),

        Type::Str => quote!(defmt::export::str(#arg)),
        Type::IStr => quote!(defmt::export::istr(#arg)),
        Type::Char => quote!(defmt::export::char(#arg)),

        Type::Format => quote!(defmt::export::fmt(#arg)),
        Type::FormatSlice => quote!(defmt::export::fmt_slice(#arg)),
        Type::FormatArray(len) => quote!(defmt::export::fmt_array({
            let tmp: &[_; #len] = #arg;
            tmp
        })),

        Type::Debug => quote!(defmt::export::debug(#arg)),
        Type::Display => quote!(defmt::export::display(#arg)),
        Type::FormatSequence => unreachable!(),

        Type::U8Slice => quote!(defmt::export::slice(#arg)),

        // We cast to the expected array type (which should be a no-op cast) to provoke
        // a type mismatch error on mismatched lengths:
        // ``Symbol’s value as variable is void: //
        Type::U8Array(len) => quote!(defmt::export::u8_array({
            let tmp: &[u8; #len] = #arg;
            tmp
        })),

        Type::BitField(_) => {
            let all_bitfields = params.iter().filter(|param| param.index == arg_index);
            let (smallest_bit_index, largest_bit_index) =
                defmt_parser::get_max_bitfield_range(all_bitfields).unwrap();

            // indices of the lowest and the highest octet which contains bitfield-relevant data
            let lowest_byte = smallest_bit_index / 8;
            let highest_byte = (largest_bit_index - 1) / 8;
            let truncated_sz = highest_byte - lowest_byte + 1; // in bytes

            // shift away unneeded lower octet
            // TODO: create helper for shifting because readability
            match truncated_sz {
                1 => {
                    quote!(defmt::export::u8(&defmt::export::truncate((*#arg) >> (#lowest_byte * 8))))
                }
                2 => {
                    quote!(defmt::export::u16(&defmt::export::truncate((*#arg) >> (#lowest_byte * 8))))
                }
                3..=4 => {
                    quote!(defmt::export::u32(&defmt::export::truncate((*#arg) >> (#lowest_byte * 8))))
                }
                5..=8 => {
                    quote!(defmt::export::u64(&defmt::export::truncate((*#arg) >> (#lowest_byte * 8))))
                }
                9..=16 => {
                    quote!(defmt::export::u128(&defmt::export::truncate((*#arg) >> (#lowest_byte * 8))))
                }
                _ => unreachable!(),
            }
        }
    }
}
