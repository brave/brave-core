#[cfg(test)]
mod tests {
    #[allow(unused_imports)]
    use crate::*;

    // Ensure that toll-free bridged types are not marked as thread-safe.
    #[cfg(feature = "CTFont")]
    static_assertions::assert_not_impl_any!(CTFont: Send, Sync);
    #[cfg(feature = "CTFontCollection")]
    static_assertions::assert_not_impl_any!(CTFontCollection: Send, Sync);
    #[cfg(feature = "CTFontDescriptor")]
    static_assertions::assert_not_impl_any!(CTFontDescriptor: Send, Sync);
    #[cfg(feature = "CTGlyphInfo")]
    static_assertions::assert_not_impl_any!(CTGlyphInfo: Send, Sync);
}
