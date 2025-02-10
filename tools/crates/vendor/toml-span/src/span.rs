//! Provides span helpers

/// A start and end location within a toml document
#[derive(Copy, Clone, PartialEq, Eq, Default, Debug)]
pub struct Span {
    /// The start byte index
    pub start: usize,
    /// The end (exclusive) byte index
    pub end: usize,
}

impl Span {
    /// Creates a new [`Span`]
    #[inline]
    pub fn new(start: usize, end: usize) -> Self {
        Self { start, end }
    }

    /// Checks if the start and end are the same, and thus the span is empty
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.start == 0 && self.end == 0
    }
}

impl From<Span> for (usize, usize) {
    fn from(Span { start, end }: Span) -> (usize, usize) {
        (start, end)
    }
}

impl From<std::ops::Range<usize>> for Span {
    fn from(s: std::ops::Range<usize>) -> Self {
        Self {
            start: s.start,
            end: s.end,
        }
    }
}

impl From<Span> for std::ops::Range<usize> {
    fn from(s: Span) -> Self {
        Self {
            start: s.start,
            end: s.end,
        }
    }
}

/// An arbitrary `T` with additional span information
pub struct Spanned<T> {
    /// The value
    pub value: T,
    /// The span information for the value
    pub span: Span,
}

impl<T> Spanned<T> {
    /// Creates a [`Spanned`] with just the value and an empty [`Span`]
    #[inline]
    pub const fn new(value: T) -> Self {
        Self {
            value,
            span: Span { start: 0, end: 0 },
        }
    }

    /// Creates a [`Spanned`] from both a value and a [`Span`]
    #[inline]
    pub const fn with_span(value: T, span: Span) -> Self {
        Self { value, span }
    }

    /// Converts [`Self`] into its inner value
    #[inline]
    pub fn take(self) -> T {
        self.value
    }

    /// Helper to convert the value inside the Spanned
    #[inline]
    pub fn map<V>(self) -> Spanned<V>
    where
        V: From<T>,
    {
        Spanned {
            value: self.value.into(),
            span: self.span,
        }
    }
}

impl<T> Default for Spanned<T>
where
    T: Default,
{
    fn default() -> Self {
        Self {
            value: Default::default(),
            span: Span::default(),
        }
    }
}

impl<T> AsRef<T> for Spanned<T> {
    fn as_ref(&self) -> &T {
        &self.value
    }
}

impl<T> std::fmt::Debug for Spanned<T>
where
    T: std::fmt::Debug,
{
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self.value)
    }
}

impl<T> Clone for Spanned<T>
where
    T: Clone,
{
    fn clone(&self) -> Self {
        Self {
            value: self.value.clone(),
            span: self.span,
        }
    }
}

impl<T> PartialOrd for Spanned<T>
where
    T: PartialOrd,
{
    fn partial_cmp(&self, o: &Spanned<T>) -> Option<std::cmp::Ordering> {
        self.value.partial_cmp(&o.value)
    }
}

impl<T> Ord for Spanned<T>
where
    T: Ord,
{
    fn cmp(&self, o: &Spanned<T>) -> std::cmp::Ordering {
        self.value.cmp(&o.value)
    }
}

impl<T> PartialEq for Spanned<T>
where
    T: PartialEq,
{
    fn eq(&self, o: &Spanned<T>) -> bool {
        self.value == o.value
    }
}

impl<T> Eq for Spanned<T> where T: Eq {}

impl<T> PartialEq<T> for Spanned<T>
where
    T: PartialEq,
{
    fn eq(&self, o: &T) -> bool {
        &self.value == o
    }
}

impl<'de, T> crate::Deserialize<'de> for Spanned<T>
where
    T: crate::Deserialize<'de>,
{
    #[inline]
    fn deserialize(value: &mut crate::value::Value<'de>) -> Result<Self, crate::DeserError> {
        let span = value.span;
        let value = T::deserialize(value)?;
        Ok(Self { span, value })
    }
}
