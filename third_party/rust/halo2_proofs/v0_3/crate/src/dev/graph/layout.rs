use ff::Field;
use plotters::{
    coord::Shift,
    prelude::{DrawingArea, DrawingAreaErrorKind, DrawingBackend},
};
use std::collections::HashSet;
use std::ops::Range;

use crate::{
    circuit::layouter::RegionColumn,
    dev::cost::Layout,
    plonk::{Any, Circuit, Column, ConstraintSystem, FloorPlanner},
};

/// Graphical renderer for circuit layouts.
///
/// Cells that have been assigned to by the circuit will be shaded. If any cells are
/// assigned to more than once (which is usually a mistake), they will be shaded darker
/// than the surrounding cells.
///
/// # Examples
///
/// ```ignore
/// use halo2_proofs::dev::CircuitLayout;
/// use plotters::prelude::*;
///
/// let drawing_area = BitMapBackend::new("example-circuit-layout.png", (1024, 768))
///     .into_drawing_area();
/// drawing_area.fill(&WHITE).unwrap();
/// let drawing_area = drawing_area
///     .titled("Example Circuit Layout", ("sans-serif", 60))
///     .unwrap();
///
/// let circuit = MyCircuit::default();
/// let k = 5; // Suitable size for MyCircuit
/// CircuitLayout::default().render(k, &circuit, &drawing_area).unwrap();
/// ```
#[derive(Debug, Default)]
pub struct CircuitLayout {
    hide_labels: bool,
    mark_equality_cells: bool,
    show_equality_constraints: bool,
    view_width: Option<Range<usize>>,
    view_height: Option<Range<usize>>,
}

impl CircuitLayout {
    /// Sets the visibility of region labels.
    ///
    /// The default is to show labels.
    pub fn show_labels(mut self, show: bool) -> Self {
        self.hide_labels = !show;
        self
    }

    /// Marks cells involved in equality constraints, in red.
    ///
    /// The default is to not mark these cells.
    pub fn mark_equality_cells(mut self, show: bool) -> Self {
        self.mark_equality_cells = show;
        self
    }

    /// Draws red lines between equality-constrained cells.
    ///
    /// The default is to not show these, as they can get _very_ messy.
    pub fn show_equality_constraints(mut self, show: bool) -> Self {
        self.show_equality_constraints = show;
        self
    }

    /// Sets the view width for this layout, as a number of columns.
    pub fn view_width(mut self, width: Range<usize>) -> Self {
        self.view_width = Some(width);
        self
    }

    /// Sets the view height for this layout, as a number of rows.
    pub fn view_height(mut self, height: Range<usize>) -> Self {
        self.view_height = Some(height);
        self
    }

    /// Renders the given circuit on the given drawing area.
    pub fn render<F: Field, ConcreteCircuit: Circuit<F>, DB: DrawingBackend>(
        self,
        k: u32,
        circuit: &ConcreteCircuit,
        drawing_area: &DrawingArea<DB, Shift>,
    ) -> Result<(), DrawingAreaErrorKind<DB::ErrorType>> {
        use plotters::coord::types::RangedCoordusize;
        use plotters::prelude::*;

        let n = 1 << k;
        // Collect the layout details.
        let mut cs = ConstraintSystem::default();
        let config = ConcreteCircuit::configure(&mut cs);
        let mut layout = Layout::new(k, n, cs.num_selectors);
        ConcreteCircuit::FloorPlanner::synthesize(
            &mut layout,
            circuit,
            config,
            cs.constants.clone(),
        )
        .unwrap();
        let (cs, selector_polys) = cs.compress_selectors(layout.selectors);
        let non_selector_fixed_columns = cs.num_fixed_columns - selector_polys.len();

        // Figure out what order to render the columns in.
        // TODO: For now, just render them in the order they were configured.
        let total_columns = cs.num_instance_columns + cs.num_advice_columns + cs.num_fixed_columns;
        let column_index = |cs: &ConstraintSystem<F>, column: RegionColumn| {
            let column: Column<Any> = match column {
                RegionColumn::Column(col) => col,
                RegionColumn::Selector(selector) => cs.selector_map[selector.0].into(),
            };
            column.index()
                + match column.column_type() {
                    Any::Instance => 0,
                    Any::Advice => cs.num_instance_columns,
                    Any::Fixed => cs.num_instance_columns + cs.num_advice_columns,
                }
        };

        let view_width = self.view_width.unwrap_or(0..total_columns);
        let view_height = self.view_height.unwrap_or(0..n);
        let view_bottom = view_height.end;

        // Prepare the grid layout. We render a red background for advice columns, white for
        // instance columns, and blue for fixed columns (with a darker blue for selectors).
        let root =
            drawing_area.apply_coord_spec(Cartesian2d::<RangedCoordusize, RangedCoordusize>::new(
                view_width,
                view_height,
                drawing_area.get_pixel_range(),
            ));
        root.draw(&Rectangle::new(
            [(0, 0), (total_columns, view_bottom)],
            ShapeStyle::from(&WHITE).filled(),
        ))?;
        root.draw(&Rectangle::new(
            [
                (cs.num_instance_columns, 0),
                (cs.num_instance_columns + cs.num_advice_columns, view_bottom),
            ],
            ShapeStyle::from(&RED.mix(0.2)).filled(),
        ))?;
        root.draw(&Rectangle::new(
            [
                (cs.num_instance_columns + cs.num_advice_columns, 0),
                (total_columns, view_bottom),
            ],
            ShapeStyle::from(&BLUE.mix(0.2)).filled(),
        ))?;
        {
            root.draw(&Rectangle::new(
                [
                    (
                        cs.num_instance_columns
                            + cs.num_advice_columns
                            + non_selector_fixed_columns,
                        0,
                    ),
                    (total_columns, view_bottom),
                ],
                ShapeStyle::from(&BLUE.mix(0.1)).filled(),
            ))?;
        }

        // Mark the unusable rows of the circuit.
        let usable_rows = n - (cs.blinding_factors() + 1);
        if view_bottom > usable_rows {
            root.draw(&Rectangle::new(
                [(0, usable_rows), (total_columns, view_bottom)],
                ShapeStyle::from(&RED.mix(0.4)).filled(),
            ))?;
        }

        root.draw(&Rectangle::new(
            [(0, 0), (total_columns, view_bottom)],
            BLACK,
        ))?;

        let draw_region = |root: &DrawingArea<_, _>, top_left, bottom_right| {
            root.draw(&Rectangle::new(
                [top_left, bottom_right],
                ShapeStyle::from(&WHITE).filled(),
            ))?;
            root.draw(&Rectangle::new(
                [top_left, bottom_right],
                ShapeStyle::from(&RED.mix(0.2)).filled(),
            ))?;
            root.draw(&Rectangle::new(
                [top_left, bottom_right],
                ShapeStyle::from(&GREEN.mix(0.2)).filled(),
            ))?;
            root.draw(&Rectangle::new([top_left, bottom_right], BLACK))?;
            Ok(())
        };

        let draw_cell = |root: &DrawingArea<_, _>, column, row| {
            root.draw(&Rectangle::new(
                [(column, row), (column + 1, row + 1)],
                ShapeStyle::from(&BLACK.mix(0.1)).filled(),
            ))
        };

        // Render the regions!
        let mut labels = if self.hide_labels { None } else { Some(vec![]) };
        for region in &layout.regions {
            if let Some(offset) = region.offset {
                // Sort the region's columns according to the defined ordering.
                let mut columns: Vec<_> = region.columns.iter().cloned().collect();
                columns.sort_unstable_by_key(|a| column_index(&cs, *a));

                // Render contiguous parts of the same region as a single box.
                let mut width = None;
                for column in columns {
                    let column = column_index(&cs, column);
                    match width {
                        Some((start, end)) if end == column => width = Some((start, end + 1)),
                        Some((start, end)) => {
                            draw_region(&root, (start, offset), (end, offset + region.rows))?;
                            if let Some(labels) = &mut labels {
                                labels.push((region.name.clone(), (start, offset)));
                            }
                            width = Some((column, column + 1));
                        }
                        None => width = Some((column, column + 1)),
                    }
                }

                // Render the last part of the region.
                if let Some((start, end)) = width {
                    draw_region(&root, (start, offset), (end, offset + region.rows))?;
                    if let Some(labels) = &mut labels {
                        labels.push((region.name.clone(), (start, offset)));
                    }
                }
            }
        }

        // Darken the cells of the region that have been assigned to.
        for region in layout.regions {
            for (column, row) in region.cells {
                draw_cell(&root, column_index(&cs, column), row)?;
            }
        }

        // Darken any loose cells that have been assigned to.
        for (column, row) in layout.loose_cells {
            draw_cell(&root, column_index(&cs, column), row)?;
        }

        // Mark equality-constrained cells.
        if self.mark_equality_cells {
            let mut cells = HashSet::new();
            for (l_col, l_row, r_col, r_row) in &layout.equality {
                let l_col = column_index(&cs, (*l_col).into());
                let r_col = column_index(&cs, (*r_col).into());

                // Deduplicate cells.
                cells.insert((l_col, *l_row));
                cells.insert((r_col, *r_row));
            }

            for (col, row) in cells {
                root.draw(&Rectangle::new(
                    [(col, row), (col + 1, row + 1)],
                    ShapeStyle::from(&RED.mix(0.5)).filled(),
                ))?;
            }
        }

        // Draw lines between equality-constrained cells.
        if self.show_equality_constraints {
            for (l_col, l_row, r_col, r_row) in &layout.equality {
                let l_col = column_index(&cs, (*l_col).into());
                let r_col = column_index(&cs, (*r_col).into());
                root.draw(&PathElement::new(
                    [(l_col, *l_row), (r_col, *r_row)],
                    ShapeStyle::from(&RED),
                ))?;
            }
        }

        // Add a line showing the total used rows.
        root.draw(&PathElement::new(
            [(0, layout.total_rows), (total_columns, layout.total_rows)],
            ShapeStyle::from(&BLACK),
        ))?;

        // Render labels last, on top of everything else.
        if let Some(labels) = labels {
            for (label, top_left) in labels {
                root.draw(
                    &(EmptyElement::at(top_left)
                        + Text::new(label, (10, 10), ("sans-serif", 15.0).into_font())),
                )?;
            }
            root.draw(
                &(EmptyElement::at((0, layout.total_rows))
                    + Text::new(
                        format!("{} used rows", layout.total_rows),
                        (10, 10),
                        ("sans-serif", 15.0).into_font(),
                    )),
            )?;
            root.draw(
                &(EmptyElement::at((0, usable_rows))
                    + Text::new(
                        format!("{} usable rows", usable_rows),
                        (10, 10),
                        ("sans-serif", 15.0).into_font(),
                    )),
            )?;
        }
        Ok(())
    }
}
