//! Facilities to produce the unified diff format.
//!
//! Originally based on <https://github.com/pascalkuthe/imara-diff/pull/14>.

/// Defines the size of the context printed before and after each change.
///
/// Similar to the `-U` option in git diff or gnu-diff. If the context overlaps
/// with previous or next change, the context gets reduced accordingly.
#[derive(Debug, Copy, Clone, Hash, PartialEq, Eq, Ord, PartialOrd)]
pub struct ContextSize {
    /// Defines the size of the context printed before and after each change.
    symmetrical: u32,
}

impl Default for ContextSize {
    fn default() -> Self {
        ContextSize::symmetrical(3)
    }
}

/// Instantiation
impl ContextSize {
    /// Create a symmetrical context with `n` lines before and after a changed hunk.
    pub fn symmetrical(n: u32) -> Self {
        ContextSize { symmetrical: n }
    }
}

/// Represents the type of a line in a unified diff.
#[doc(alias = "git2")]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Ord, PartialOrd)]
pub enum DiffLineKind {
    /// A line that exists in both the old and the new version, added based on [`ContextSize`].
    Context,
    /// A line that was added in the new version.
    Add,
    /// A line that was removed from the old version.
    Remove,
}

/// Holds information about a unified diff hunk, specifically with respect to line numbers.
#[derive(Default, Debug, Copy, Clone, PartialEq, Eq, Hash, Ord, PartialOrd)]
pub struct HunkHeader {
    /// The 1-based start position in the 'before' lines.
    pub before_hunk_start: u32,
    /// The size of the 'before' hunk in lines.
    pub before_hunk_len: u32,
    /// The 1-based start position in the 'after' lines.
    pub after_hunk_start: u32,
    /// The size of the 'after' hunk in lines.
    pub after_hunk_len: u32,
}

/// An adapter with [`ConsumeHunk`] implementation to call a delegate which receives each stringified hunk.
pub struct ConsumeBinaryHunk<'a, D> {
    /// The newline to use to separate lines if these don't yet contain a newline.
    /// It should also be used to separate the stringified header from the hunk itself.
    pub newline: &'a str,
    /// The delegate to receive stringified hunks.
    pub delegate: D,

    header_buf: String,
    hunk_buf: Vec<u8>,
}

/// A trait for use in conjunction with [`ConsumeBinaryHunk`].
pub trait ConsumeBinaryHunkDelegate {
    /// Consume a single `hunk` in unified diff format, along with its `header_str` that already has a trailing newline added based
    /// on the parent [`ConsumeBinaryHunk`] configuration, also in unified diff format.
    /// The `header` is the data used to produce `header_str`.
    fn consume_binary_hunk(&mut self, header: HunkHeader, header_str: &str, hunk: &[u8]) -> std::io::Result<()>;
}

/// A utility trait for use in [`UnifiedDiff`](super::UnifiedDiff).
pub trait ConsumeHunk {
    /// The item this instance produces after consuming all hunks.
    type Out;

    /// Consume a single hunk which is represented by its `lines`, each of which with a `DiffLineKind` value
    /// to know if it's added, removed or context.
    /// The `header` specifies hunk offsets, which positions the `lines` in the old and new file respectively.
    ///
    /// Note that the [`UnifiedDiff`](super::UnifiedDiff) sink will wrap its output in an [`std::io::Result`].
    /// After this method returned its first error, it will not be called anymore.
    fn consume_hunk(&mut self, header: HunkHeader, lines: &[(DiffLineKind, &[u8])]) -> std::io::Result<()>;

    /// Called after the last hunk is consumed to produce an output.
    fn finish(self) -> Self::Out;
}

pub(super) mod impls;
