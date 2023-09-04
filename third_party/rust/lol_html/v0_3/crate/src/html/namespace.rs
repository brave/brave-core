#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub enum Namespace {
    Html,
    Svg,
    MathML,
}

impl Namespace {
    #[inline]
    pub fn uri(self) -> &'static str {
        use Namespace::*;

        // NOTE: https://infra.spec.whatwg.org/#namespaces
        match self {
            Html => "http://www.w3.org/1999/xhtml",
            Svg => "http://www.w3.org/2000/svg",
            MathML => "http://www.w3.org/1998/Math/MathML",
        }
    }
}

impl Default for Namespace {
    #[inline]
    fn default() -> Self {
        Namespace::Html
    }
}
