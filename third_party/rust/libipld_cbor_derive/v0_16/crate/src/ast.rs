use proc_macro2::TokenStream;

#[derive(Clone, Debug)]
pub struct TokenStreamEq(pub TokenStream);

impl std::ops::Deref for TokenStreamEq {
    type Target = TokenStream;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl PartialEq for TokenStreamEq {
    fn eq(&self, other: &Self) -> bool {
        self.0.to_string() == other.0.to_string()
    }
}

impl Eq for TokenStreamEq {}
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum SchemaType {
    Struct(Struct),
    Union(Union),
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Struct {
    pub name: syn::Ident,
    pub generics: Option<syn::Generics>,
    pub rename: Option<String>,
    pub fields: Vec<StructField>,
    pub repr: StructRepr,
    pub pat: TokenStreamEq,
    pub construct: TokenStreamEq,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct StructField {
    pub name: syn::Member,
    pub rename: Option<String>,
    pub default: Option<Box<syn::Expr>>,
    pub binding: syn::Ident,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum StructRepr {
    Map,
    Tuple,
    Value,
    Null,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Union {
    pub name: syn::Ident,
    pub generics: syn::Generics,
    pub variants: Vec<Struct>,
    pub repr: UnionRepr,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum UnionRepr {
    Keyed,
    Kinded,
    String,
    Int,
    IntTuple,
}
