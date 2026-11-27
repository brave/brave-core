//! Optimization for format descriptions.
//!
//! The tree of all items is walked recursively and optimized in-place. Optimizations are ordered so
//! that their effects are consumed by later optimizations in a single pass. Children are optimized
//! before their parent, so sibling-level interactions are the only concern at each level.
//!
//! Each optimization function accepts `self` mutably and returns whether it modified the tree. Note
//! that optimizations *must not* affect runtime behavior in terms of formatting output, accepted
//! input when parsing, or output from the parser.

use std::mem;

use super::{Component, OwnedFormatItem, OwnedFormatItemInner};

impl OwnedFormatItem {
    pub(crate) fn optimize(&mut self) {
        self.inner.optimize();
    }
}

impl OwnedFormatItemInner {
    pub(crate) fn optimize(&mut self) {
        // Walk the tree and optimize all children.
        match self {
            Self::Literal(_) | Self::StringLiteral(_) | Self::Component(_) => {}
            Self::Compound(items) | Self::First(items) => {
                for item in items {
                    item.optimize();
                }
            }
            Self::Optional { format: _, item } => item.optimize(),
        }

        // Run all optimizations in dependency order: group only creates opportunities that later
        // passes consume. Passes that mutate the node type (e.g., uplifting an optional) run first,
        // followed by structural passes (unnesting, cleanup), with consuming passes (merging,
        // trivial unwrapping) last.
        let passes = [
            // type-changing passes
            Self::only_formatting_uplift_optional,
            Self::only_formatting_uplift_first,
            Self::only_formatting_eliminate_end,
            // structural passes: inline nested containers, remove no-op children
            Self::unnest_nested_compounds,
            Self::unnest_nested_first,
            Self::compound_containing_empty_string,
            // consuming passes: merge siblings, unwrap trivial wrappers
            Self::merge_consecutive_literals,
            Self::unnest_trivial_compounds,
            Self::unnest_first_only_one,
        ];
        for pass in passes {
            pass(self);
        }
    }

    const fn no_op() -> Self {
        Self::StringLiteral(String::new())
    }

    /// When there are multiple consecutive literals, they can be merged into a single literal.
    ///
    /// As there are both UTF-8 and non-UTF-8 literals, the output is UTF-8 if and only if both
    /// literals are as well.
    fn merge_consecutive_literals(&mut self) -> bool {
        let Self::Compound(items) = self else {
            return false;
        };

        let mut something_was_changed = false;
        let mut idx = 1;
        while idx < items.len() {
            // Safety: `idx - 1` is not equal to `idx` and both are in-bounds.
            let pair = unsafe { items.get_disjoint_unchecked_mut([idx - 1, idx]) };

            match pair {
                [Self::Literal(a), Self::Literal(b)] => {
                    a.append(b);
                    items.remove(idx);
                    something_was_changed = true;
                }
                [Self::Literal(a), Self::StringLiteral(b)] => {
                    a.extend(b.as_bytes());
                    items.remove(idx);
                    something_was_changed = true;
                }
                [item @ Self::StringLiteral(_), Self::Literal(b)] => {
                    let Self::StringLiteral(a) = item else {
                        unreachable!()
                    };
                    let mut bytes = a.as_bytes().to_vec();
                    bytes.append(b);
                    *item = Self::Literal(bytes);
                    items.remove(idx);
                    something_was_changed = true;
                }
                [Self::StringLiteral(a), Self::StringLiteral(b)] => {
                    a.push_str(b);
                    items.remove(idx);
                    something_was_changed = true;
                }
                _ => idx += 1,
            }
        }

        something_was_changed
    }

    /// When a compound item only contains a single item, it can be replaced with that item.
    fn unnest_trivial_compounds(&mut self) -> bool {
        if let Self::Compound(items) = self
            && items.len() == 1
            && let Some(item) = items.pop()
        {
            *self = item;
            true
        } else {
            false
        }
    }

    /// When a compound item contains another compound item, the latter can be inlined into the
    /// former.
    fn unnest_nested_compounds(&mut self) -> bool {
        let Self::Compound(items) = self else {
            return false;
        };

        let mut idx = 0;
        let mut something_was_changed = false;
        while idx < items.len() {
            if let Self::Compound(inner_items) = &mut items[idx] {
                let inner_items = mem::take(inner_items);
                items.splice(idx..=idx, inner_items).for_each(drop);
                something_was_changed = true;
            } else {
                idx += 1;
            }
        }

        something_was_changed
    }

    /// When a first item only contains a single item, it can be replaced with that item.
    fn unnest_first_only_one(&mut self) -> bool {
        if let Self::First(items) = self
            && items.len() == 1
            && let Some(item) = items.pop()
        {
            *self = item;
            true
        } else {
            false
        }
    }

    /// When a first item contains another first item, the latter can be inlined into the former.
    fn unnest_nested_first(&mut self) -> bool {
        let Self::First(items) = self else {
            return false;
        };

        let mut idx = 0;
        let mut something_was_changed = false;
        while idx < items.len() {
            if let Self::First(inner_items) = &mut items[idx] {
                let inner_items = mem::take(inner_items);
                items.splice(idx..=idx, inner_items).for_each(drop);
                something_was_changed = true;
            } else {
                idx += 1;
            }
        }

        something_was_changed
    }

    /// When formatting is enabled but parsing is not, the behavior of an optional item is known
    /// ahead of time. If it is formatted, the optional item can be replaced with its inner item. If
    /// it is not formatted, it can be replace with a no-op (that will likely be removed in a later
    /// pass).
    fn only_formatting_uplift_optional(&mut self) -> bool {
        // This optimization only makes sense when *only* formatting is enabled, as otherwise the
        // optional item may be needed for parsing.
        if !cfg!(feature = "formatting") || cfg!(feature = "parsing") {
            return false;
        }

        let Self::Optional { format, item } = self else {
            return false;
        };

        let item = if *format {
            mem::replace(item.as_mut(), Self::no_op())
        } else {
            Self::no_op()
        };

        *self = item;
        true
    }

    /// When formatting is enabled but parsing is not, the behavior of a first item is known ahead
    /// of time. It can be replaced with its first item, as the first item will always be the
    /// one that is formatted.
    fn only_formatting_uplift_first(&mut self) -> bool {
        // This optimization only makes sense when *only* formatting is enabled, as otherwise the
        // remaining items may be needed for parsing.
        if !cfg!(feature = "formatting") || cfg!(feature = "parsing") {
            return false;
        }

        let Self::First(items) = self else {
            return false;
        };

        *self = items.remove(0);
        true
    }

    fn only_formatting_eliminate_end(&mut self) -> bool {
        // This optimization only makes sense when *only* formatting is enabled, as otherwise the
        // remaining items may be needed for parsing.
        if !cfg!(feature = "formatting") || cfg!(feature = "parsing") {
            return false;
        }

        if let Self::Component(Component::End(_)) = self {
            *self = Self::no_op();
            true
        } else {
            false
        }
    }

    /// When a compound item contains an empty string literal, it can be removed as it has no
    /// effect.
    fn compound_containing_empty_string(&mut self) -> bool {
        let Self::Compound(items) = self else {
            return false;
        };

        let mut idx = 0;
        let mut something_was_changed = false;
        while idx < items.len() {
            if let Self::StringLiteral(s) = &items[idx]
                && s.is_empty()
            {
                items.remove(idx);
                something_was_changed = true;
            } else {
                idx += 1;
            }
        }

        something_was_changed
    }
}
