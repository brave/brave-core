use std::{
    cmp,
    collections::{BTreeSet, HashMap},
    ops::Range,
};

use super::{RegionColumn, RegionShape};
use crate::{circuit::RegionStart, plonk::Any};

/// A region allocated within a column.
#[derive(Clone, Default, Debug, PartialEq, Eq)]
struct AllocatedRegion {
    // The starting position of the region.
    start: usize,
    // The length of the region.
    length: usize,
}

impl Ord for AllocatedRegion {
    fn cmp(&self, other: &Self) -> cmp::Ordering {
        self.start.cmp(&other.start)
    }
}

impl PartialOrd for AllocatedRegion {
    fn partial_cmp(&self, other: &Self) -> Option<cmp::Ordering> {
        Some(self.cmp(other))
    }
}

/// An area of empty space within a column.
pub(crate) struct EmptySpace {
    // The starting position (inclusive) of the empty space.
    start: usize,
    // The ending position (exclusive) of the empty space, or `None` if unbounded.
    end: Option<usize>,
}

impl EmptySpace {
    pub(crate) fn range(&self) -> Option<Range<usize>> {
        self.end.map(|end| self.start..end)
    }
}

/// Allocated rows within a column.
///
/// This is a set of [a_start, a_end) pairs representing disjoint allocated intervals.
#[derive(Clone, Default, Debug)]
pub struct Allocations(BTreeSet<AllocatedRegion>);

impl Allocations {
    /// Returns the row that forms the unbounded unallocated interval [row, None).
    pub(crate) fn unbounded_interval_start(&self) -> usize {
        self.0
            .iter()
            .last()
            .map(|r| r.start + r.length)
            .unwrap_or(0)
    }

    /// Return all the *unallocated* nonempty intervals intersecting [start, end).
    ///
    /// `end = None` represents an unbounded end.
    pub(crate) fn free_intervals(
        &self,
        start: usize,
        end: Option<usize>,
    ) -> impl Iterator<Item = EmptySpace> + '_ {
        self.0
            .iter()
            .map(Some)
            .chain(Some(None))
            .scan(start, move |row, region| {
                Some(if let Some(region) = region {
                    if end.map(|end| region.start >= end).unwrap_or(false) {
                        None
                    } else {
                        let ret = if *row < region.start {
                            Some(EmptySpace {
                                start: *row,
                                end: Some(region.start),
                            })
                        } else {
                            None
                        };

                        *row = cmp::max(*row, region.start + region.length);

                        ret
                    }
                } else if end.map(|end| *row < end).unwrap_or(true) {
                    Some(EmptySpace { start: *row, end })
                } else {
                    None
                })
            })
            .flatten()
    }
}

/// Allocated rows within a circuit.
pub type CircuitAllocations = HashMap<RegionColumn, Allocations>;

/// - `start` is the current start row of the region (not of this column).
/// - `slack` is the maximum number of rows the start could be moved down, taking into
///   account prior columns.
fn first_fit_region(
    column_allocations: &mut CircuitAllocations,
    region_columns: &[RegionColumn],
    region_length: usize,
    start: usize,
    slack: Option<usize>,
) -> Option<usize> {
    let (c, remaining_columns) = match region_columns.split_first() {
        Some(cols) => cols,
        None => return Some(start),
    };
    let end = slack.map(|slack| start + region_length + slack);

    // Iterate over the unallocated non-empty intervals in c that intersect [start, end).
    for space in column_allocations
        .entry(*c)
        .or_default()
        .clone()
        .free_intervals(start, end)
    {
        // Do we have enough room for this column of the region in this interval?
        let s_slack = space
            .end
            .map(|end| (end as isize - space.start as isize) - region_length as isize);
        if let Some((slack, s_slack)) = slack.zip(s_slack) {
            assert!(s_slack <= slack as isize);
        }
        if s_slack.unwrap_or(0) >= 0 {
            let row = first_fit_region(
                column_allocations,
                remaining_columns,
                region_length,
                space.start,
                s_slack.map(|s| s as usize),
            );
            if let Some(row) = row {
                if let Some(end) = end {
                    assert!(row + region_length <= end);
                }
                column_allocations
                    .get_mut(c)
                    .unwrap()
                    .0
                    .insert(AllocatedRegion {
                        start: row,
                        length: region_length,
                    });
                return Some(row);
            }
        }
    }

    // No placement worked; the caller will need to try other possibilities.
    None
}

/// Positions the regions starting at the earliest row for which none of the columns are
/// in use, taking into account gaps between earlier regions.
fn slot_in(
    region_shapes: Vec<RegionShape>,
) -> (Vec<(RegionStart, RegionShape)>, CircuitAllocations) {
    // Tracks the empty regions for each column.
    let mut column_allocations: CircuitAllocations = Default::default();

    let regions = region_shapes
        .into_iter()
        .map(|region| {
            // Sort the region's columns to ensure determinism.
            // - An unstable sort is fine, because region.columns() returns a set.
            // - The sort order relies on Column's Ord implementation!
            let mut region_columns: Vec<_> = region.columns().iter().cloned().collect();
            region_columns.sort_unstable();

            let region_start = first_fit_region(
                &mut column_allocations,
                &region_columns,
                region.row_count(),
                0,
                None,
            )
            .expect("We can always fit a region somewhere");

            (region_start.into(), region)
        })
        .collect();

    // Return the column allocations for potential further processing.
    (regions, column_allocations)
}

/// Sorts the regions by advice area and then lays them out with the [`slot_in`] strategy.
pub fn slot_in_biggest_advice_first(
    region_shapes: Vec<RegionShape>,
) -> (Vec<RegionStart>, CircuitAllocations) {
    let mut sorted_regions: Vec<_> = region_shapes.into_iter().collect();
    let sort_key = |shape: &RegionShape| {
        // Count the number of advice columns
        let advice_cols = shape
            .columns()
            .iter()
            .filter(|c| match c {
                RegionColumn::Column(c) => matches!(c.column_type(), Any::Advice),
                _ => false,
            })
            .count();
        // Sort by advice area (since this has the most contention).
        advice_cols * shape.row_count()
    };

    // This used to incorrectly use `sort_unstable_by_key` with non-unique keys, which gave
    // output that differed between 32-bit and 64-bit platforms, and potentially between Rust
    // versions.
    // We now use `sort_by_cached_key` with non-unique keys, and rely on `region_shapes`
    // being sorted by region index (which we also rely on below to return `RegionStart`s
    // in the correct order).
    #[cfg(not(feature = "floor-planner-v1-legacy-pdqsort"))]
    sorted_regions.sort_by_cached_key(sort_key);

    // To preserve compatibility, when the "floor-planner-v1-legacy-pdqsort" feature is enabled,
    // we use a copy of the pdqsort implementation from the Rust 1.56.1 standard library, fixed
    // to its behaviour on 64-bit platforms.
    // https://github.com/rust-lang/rust/blob/1.56.1/library/core/src/slice/mod.rs#L2365-L2402
    #[cfg(feature = "floor-planner-v1-legacy-pdqsort")]
    halo2_legacy_pdqsort::sort::quicksort(&mut sorted_regions, |a, b| sort_key(a).lt(&sort_key(b)));

    sorted_regions.reverse();

    // Lay out the sorted regions.
    let (mut regions, column_allocations) = slot_in(sorted_regions);

    // Un-sort the regions so they match the original indexing.
    regions.sort_unstable_by_key(|(_, region)| region.region_index().0);
    let regions = regions.into_iter().map(|(start, _)| start).collect();

    (regions, column_allocations)
}

#[test]
fn test_slot_in() {
    use crate::plonk::Column;

    let regions = vec![
        RegionShape {
            region_index: 0.into(),
            columns: vec![Column::new(0, Any::Advice), Column::new(1, Any::Advice)]
                .into_iter()
                .map(|a| a.into())
                .collect(),
            row_count: 15,
        },
        RegionShape {
            region_index: 1.into(),
            columns: vec![Column::new(2, Any::Advice)]
                .into_iter()
                .map(|a| a.into())
                .collect(),
            row_count: 10,
        },
        RegionShape {
            region_index: 2.into(),
            columns: vec![Column::new(2, Any::Advice), Column::new(0, Any::Advice)]
                .into_iter()
                .map(|a| a.into())
                .collect(),
            row_count: 10,
        },
    ];
    assert_eq!(
        slot_in(regions)
            .0
            .into_iter()
            .map(|(i, _)| i)
            .collect::<Vec<_>>(),
        vec![0.into(), 0.into(), 15.into()]
    );
}
