//! Table elements within a wasm module.

use crate::emit::{Emit, EmitContext};
use crate::parse::IndicesToIds;
use crate::tombstone_arena::{Id, Tombstone, TombstoneArena};
use crate::{ir::Value, ConstExpr, FunctionId, Module, RefType, Result, TableId, ValType};
use anyhow::{bail, Context};

/// A passive element segment identifier
pub type ElementId = Id<Element>;

/// A passive segment which contains a list of functions
#[derive(Debug)]
pub struct Element {
    id: Id<Element>,
    /// The kind of the element segment.
    pub kind: ElementKind,
    /// The initial elements of the element segment.
    pub items: ElementItems,
    /// The name of this element, used for debugging purposes in the `name`
    /// custom section.
    pub name: Option<String>,
}

/// The kind of element segment.
#[derive(Debug, Copy, Clone)]
pub enum ElementKind {
    /// The element segment is passive.
    Passive,
    /// The element segment is declared.
    Declared,
    /// The element segment is active.
    Active {
        /// The ID of the table being initialized.
        table: TableId,
        /// A constant defining an offset into that table.
        offset: ConstExpr,
    },
}

/// Represents the items of an element segment.
#[derive(Debug, Clone)]
pub enum ElementItems {
    /// This element contains function indices.
    Functions(Vec<FunctionId>),
    /// This element contains constant expressions used to initialize the table.
    Expressions(RefType, Vec<ConstExpr>),
}

impl Element {
    /// Get this segment's id
    pub fn id(&self) -> Id<Element> {
        self.id
    }
}

impl Tombstone for Element {
    fn on_delete(&mut self) {
        // Nothing to do here
    }
}

/// All element segments of a wasm module, used to initialize `anyfunc` tables,
/// used as function pointers.
#[derive(Debug, Default)]
pub struct ModuleElements {
    arena: TombstoneArena<Element>,
}

impl ModuleElements {
    /// Get an element associated with an ID
    pub fn get(&self, id: ElementId) -> &Element {
        &self.arena[id]
    }

    /// Get an element associated with an ID
    pub fn get_mut(&mut self, id: ElementId) -> &mut Element {
        &mut self.arena[id]
    }

    /// Delete an elements entry from this module.
    ///
    /// It is up to you to ensure that all references to this deleted element
    /// are removed.
    pub fn delete(&mut self, id: ElementId) {
        self.arena.delete(id);
    }

    /// Get a shared reference to this module's elements.
    pub fn iter(&self) -> impl Iterator<Item = &Element> {
        self.arena.iter().map(|(_, f)| f)
    }

    /// Get a mutable reference to this module's elements.
    pub fn iter_mut(&mut self) -> impl Iterator<Item = &mut Element> {
        self.arena.iter_mut().map(|(_, f)| f)
    }

    /// Add an element segment
    pub fn add(&mut self, kind: ElementKind, items: ElementItems) -> ElementId {
        let id = self.arena.next_id();
        let id2 = self.arena.alloc(Element {
            id,
            kind,
            items,
            name: None,
        });
        debug_assert_eq!(id, id2);
        id
    }
}

impl Module {
    /// Parses a raw was section into a fully-formed `ModuleElements` instance.
    pub(crate) fn parse_elements(
        &mut self,
        section: wasmparser::ElementSectionReader,
        ids: &mut IndicesToIds,
    ) -> Result<()> {
        log::debug!("parse element section");
        for (i, segment) in section.into_iter().enumerate() {
            let element = segment?;

            let items = match element.items {
                wasmparser::ElementItems::Functions(funcs) => {
                    let mut function_ids = Vec::with_capacity(funcs.count() as usize);
                    for func in funcs {
                        match ids.get_func(func?) {
                            Ok(func) => function_ids.push(func),
                            Err(_) => bail!("invalid function index in element segment {}", i),
                        }
                    }
                    ElementItems::Functions(function_ids)
                }
                wasmparser::ElementItems::Expressions(ref_type, items) => {
                    let ty = match ref_type {
                        wasmparser::RefType::FUNCREF => RefType::Funcref,
                        wasmparser::RefType::EXTERNREF => RefType::Externref,
                        _ => bail!("unsupported ref type in element segment {}", i),
                    };
                    let mut const_exprs = Vec::with_capacity(items.count() as usize);
                    for item in items {
                        let const_expr = item?;
                        let expr = ConstExpr::eval(&const_expr, ids).with_context(|| {
                            format!(
                                "Failed to evaluate a const expr in element segment {}:\n{:?}",
                                i, const_expr
                            )
                        })?;
                        const_exprs.push(expr);
                    }
                    ElementItems::Expressions(ty, const_exprs)
                }
            };

            let id = self.elements.arena.next_id();

            let kind = match element.kind {
                wasmparser::ElementKind::Passive => ElementKind::Passive,
                wasmparser::ElementKind::Declared => ElementKind::Declared,
                wasmparser::ElementKind::Active {
                    table_index,
                    offset_expr,
                } => {
                    // TODO: Why table_index is Option?
                    let table_id = ids.get_table(table_index.unwrap_or_default())?;
                    let table = self.tables.get_mut(table_id);
                    table.elem_segments.insert(id);

                    let offset = ConstExpr::eval(&offset_expr, ids).with_context(|| {
                        format!("failed to evaluate the offset of element {}", i)
                    })?;
                    if table.table64 {
                        match offset {
                            ConstExpr::Value(Value::I64(_)) => {}
                            ConstExpr::Global(global)
                                if self.globals.get(global).ty == ValType::I64 => {}
                            _ => bail!(
                                "element {} is active for 64-bit table but has non-i64 offset",
                                i
                            ),
                        }
                    } else {
                        match offset {
                            ConstExpr::Value(Value::I32(_)) => {}
                            ConstExpr::Global(global)
                                if self.globals.get(global).ty == ValType::I32 => {}
                            _ => bail!(
                                "element {} is active for 32-bit table but has non-i32 offset",
                                i
                            ),
                        }
                    }

                    ElementKind::Active {
                        table: table_id,
                        offset,
                    }
                }
            };
            self.elements.arena.alloc(Element {
                id,
                kind,
                items,
                name: None,
            });
            ids.push_element(id);
        }
        Ok(())
    }
}

impl Emit for ModuleElements {
    fn emit(&self, cx: &mut EmitContext) {
        if self.arena.len() == 0 {
            return;
        }

        let mut wasm_element_section = wasm_encoder::ElementSection::new();

        for (id, element) in self.arena.iter() {
            cx.indices.push_element(id);

            match &element.items {
                ElementItems::Functions(function_ids) => {
                    let idx = function_ids
                        .iter()
                        .map(|&func| cx.indices.get_func_index(func))
                        .collect::<Vec<_>>();
                    let els = wasm_encoder::Elements::Functions(&idx);
                    emit_elem(cx, &mut wasm_element_section, &element.kind, els);
                }
                ElementItems::Expressions(ty, const_exprs) => {
                    let ref_type = match ty {
                        RefType::Funcref => wasm_encoder::RefType::FUNCREF,
                        RefType::Externref => wasm_encoder::RefType::EXTERNREF,
                    };
                    let const_exprs = const_exprs
                        .iter()
                        .map(|expr| expr.to_wasmencoder_type(cx))
                        .collect::<Vec<_>>();
                    let els = wasm_encoder::Elements::Expressions(ref_type, &const_exprs);
                    emit_elem(cx, &mut wasm_element_section, &element.kind, els);
                }
            }

            fn emit_elem(
                cx: &mut EmitContext,
                wasm_element_section: &mut wasm_encoder::ElementSection,
                kind: &ElementKind,
                els: wasm_encoder::Elements,
            ) {
                match kind {
                    ElementKind::Active { table, offset } => {
                        // When the table index is 0, set this to `None` to tell `wasm-encoder` to use
                        // the backwards-compatible MVP encoding.
                        let table_index =
                            Some(cx.indices.get_table_index(*table)).filter(|&index| index != 0);
                        wasm_element_section.active(
                            table_index,
                            &offset.to_wasmencoder_type(cx),
                            els,
                        );
                    }
                    ElementKind::Passive => {
                        wasm_element_section.passive(els);
                    }
                    ElementKind::Declared => {
                        wasm_element_section.declared(els);
                    }
                }
            }
        }

        cx.wasm_module.section(&wasm_element_section);
    }
}
