use syn::{
    parse::{Parse, ParseStream},
    punctuated::Punctuated,
    token, Attribute, Expr, Ident, Token, Type, Visibility,
};

#[allow(dead_code)]
pub(super) struct Input {
    struct_attrs: Vec<Attribute>,
    vis: Visibility,
    struct_token: Token![struct],
    ident: Ident,
    colon_token: Token![:],
    ty: Type,
    brace_token: token::Brace,
    flags: Punctuated<Flag, Token![;]>,
}

impl Parse for Input {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let flags;
        Ok(Self {
            struct_attrs: Attribute::parse_outer(input)?,
            vis: input.parse()?,
            struct_token: input.parse()?,
            ident: input.parse()?,
            colon_token: input.parse()?,
            ty: input.parse()?,
            brace_token: syn::braced!(flags in input),
            flags: Punctuated::parse_terminated(&flags)?,
        })
    }
}

impl Input {
    pub(super) fn flags(&self) -> impl Iterator<Item = &Flag> {
        self.flags.iter()
    }

    pub(super) fn ident(&self) -> &Ident {
        &self.ident
    }

    pub(super) fn ty(&self) -> &Type {
        &self.ty
    }
}

#[allow(dead_code)]
pub(super) struct Flag {
    cfg_attrs: Vec<Attribute>,
    const_attrs: Vec<Attribute>,
    const_token: Token![const],
    ident: Ident,
    eq_token: Token![=],
    value: Expr,
}

impl Parse for Flag {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let const_attrs = Attribute::parse_outer(input)?;
        Ok(Self {
            cfg_attrs: extract_cfgs(&const_attrs),
            const_attrs,
            const_token: input.parse()?,
            ident: input.parse()?,
            eq_token: input.parse()?,
            value: input.parse()?,
        })
    }
}

impl Flag {
    pub(super) fn cfg_attrs(&self) -> &[Attribute] {
        &self.cfg_attrs
    }

    pub(super) fn ident(&self) -> &Ident {
        &self.ident
    }
}

fn extract_cfgs(attrs: &[Attribute]) -> Vec<Attribute> {
    let mut cfgs = vec![];

    for attr in attrs {
        if attr.path().is_ident("cfg") {
            cfgs.push(attr.clone());
        }
    }

    cfgs
}
