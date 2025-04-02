// Pub only for integration tests
#[derive(Default, Copy, Clone, Eq, PartialEq, Debug)]
pub enum Namespace {
    #[default]
    Html,
    Svg,
    MathML,
}

impl Namespace {
    #[inline]
    #[must_use]
    pub const fn uri(self) -> &'static str {
        use Namespace::{Html, MathML, Svg};

        // NOTE: https://infra.spec.whatwg.org/#namespaces
        match self {
            Html => "http://www.w3.org/1999/xhtml",
            Svg => "http://www.w3.org/2000/svg",
            MathML => "http://www.w3.org/1998/Math/MathML",
        }
    }
}
