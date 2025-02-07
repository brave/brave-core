//! Data segments within a wasm module.

use crate::emit::{Emit, EmitContext};
use crate::ir::Value;
use crate::parse::IndicesToIds;
use crate::tombstone_arena::{Id, Tombstone, TombstoneArena};
use crate::{ConstExpr, MemoryId, Module, Result, ValType};
use anyhow::{bail, Context};

/// A passive element segment identifier
pub type DataId = Id<Data>;

/// A data segment.
///
/// Every data segment has an associated value. This value gets copied into a
/// memory. It is either automatically copied into a specific memory at Wasm
/// instantiation time (active data segments) or dynamically copied into a
/// memory (or memories) via the `memory.init` instruction (passive data
/// segments). See the `kind` member and `DataKind` type for more details on the
/// active/passive distinction.
#[derive(Debug)]
pub struct Data {
    id: DataId,
    /// What kind of data segment is this? Passive or active?
    pub kind: DataKind,
    /// The data payload of this data segment.
    pub value: Vec<u8>,
    /// The name of this data, used for debugging purposes in the `name`
    /// custom section.
    pub name: Option<String>,
}

/// The kind of data segment: passive or active.
#[derive(Debug)]
pub enum DataKind {
    /// An active data segment that is automatically initialized at some address
    /// in a static memory.
    Active {
        /// The memory that this active data segment will be automatically
        /// initialized in.
        memory: MemoryId,
        /// The memory offset where this active data segment will be automatically
        /// initialized.
        offset: ConstExpr,
    },
    /// A passive data segment that must be manually initialized at a dynamic
    /// address via the `memory.init` instruction (perhaps multiple times in
    /// multiple different memories) and then manually freed when it's no longer
    /// needed via the `data.drop` instruction.
    Passive,
}

impl Tombstone for Data {
    fn on_delete(&mut self) {
        self.value = Vec::new();
    }
}

impl Data {
    /// Returns the id of this passive data segment
    pub fn id(&self) -> DataId {
        self.id
    }

    /// Is this a passive data segment?
    pub fn is_passive(&self) -> bool {
        matches!(self.kind, DataKind::Passive)
    }
}

/// All passive data sections of a wasm module, used to initialize memories via
/// various instructions.
#[derive(Debug, Default)]
pub struct ModuleData {
    arena: TombstoneArena<Data>,
}

impl ModuleData {
    /// Get an element associated with an ID
    pub fn get(&self, id: DataId) -> &Data {
        &self.arena[id]
    }

    /// Get an element associated with an ID
    pub fn get_mut(&mut self, id: DataId) -> &mut Data {
        &mut self.arena[id]
    }

    /// Delete a passive data segment from this module.
    ///
    /// It is up to you to ensure that all references to the deleted segment are
    /// removed, eg `memory.init` and `data.drop` expressions.
    pub fn delete(&mut self, id: DataId) {
        self.arena.delete(id);
    }

    /// Get a shared reference to this module's passive elements.
    pub fn iter(&self) -> impl Iterator<Item = &Data> {
        self.arena.iter().map(|(_, f)| f)
    }

    /// Add a data segment
    pub fn add(&mut self, kind: DataKind, value: Vec<u8>) -> DataId {
        let id = self.arena.next_id();
        let id2 = self.arena.alloc(Data {
            id,
            kind,
            value,
            name: None,
        });
        debug_assert_eq!(id, id2);
        id
    }

    // Note that this is inaccordance with the upstream bulk memory proposal to
    // WebAssembly and isn't currently part of the WebAssembly standard.
    pub(crate) fn emit_data_count(&self, cx: &mut EmitContext) {
        #[cfg(feature = "parallel")]
        use rayon::iter::ParallelIterator;

        if self.arena.len() == 0 {
            return;
        }

        let mut count = 0;
        let mut any_passive = false;

        for data in self.iter() {
            cx.indices.set_data_index(data.id(), count);
            count += 1;
            any_passive |= data.is_passive();
        }

        // We only emit the `DataCount` section if there are passive data
        // segments, or `data.drop` or `memory.init` instructions that use
        // (passive or active) data segments. Yes! You can use `data.drop` and
        // `memory.init` with active data segments, it just results in a runtime
        // error.
        //
        // The key is that we don't want to generate this section for MVP Wasm,
        // which has niether passive data segments, nor the `data.drop` and
        // `memory.init` instructions.
        let funcs = &cx.module.funcs;
        if any_passive
            || maybe_parallel!(funcs.(iter_local | par_iter_local))
                .any(|(_, f)| !f.used_data_segments().is_empty())
        {
            cx.wasm_module
                .section(&wasm_encoder::DataCountSection { count });
        }
    }
}

impl Module {
    /// Called when we see the data section section to create an id for all data
    /// indices
    ///
    /// Note that during function parsing all data indices less than `count` are
    /// considered valid, and it's only afterwards that we discover whether
    /// they're actually passive or not, and that property is checked during
    /// validation.
    pub(crate) fn reserve_data(&mut self, count: u32, ids: &mut IndicesToIds) {
        log::debug!("reserving space for {} data segments", count);
        for _ in 0..count {
            ids.push_data(self.data.arena.alloc_with_id(|id| Data {
                id,
                // NB: We'll update the `value` and `kind` once we actually
                // parse the data segments.
                value: Vec::new(),
                kind: DataKind::Passive,
                name: None,
            }));
        }
    }

    /// Parses a raw wasm section into a fully-formed `ModuleData` instance.
    pub(crate) fn parse_data(
        &mut self,
        section: wasmparser::DataSectionReader,
        ids: &mut IndicesToIds,
    ) -> Result<()> {
        log::debug!("parse data section");
        let preallocated = self.data.arena.len() > 0;
        for (i, segment) in section.into_iter().enumerate() {
            let segment = segment?;

            // If we had the `DataCount` section, then we already pre-allocated
            // a data segment. Otherwise, allocate one now.
            let id = if preallocated {
                ids.get_data(i as u32)?
            } else {
                let id = self.data.arena.alloc_with_id(|id| Data {
                    id,
                    value: Vec::new(),
                    kind: DataKind::Passive,
                    name: None,
                });

                ids.push_data(id);

                id
            };
            let data = self.data.get_mut(id);

            match segment.kind {
                wasmparser::DataKind::Passive => {
                    data.value = segment.data.to_vec();
                    data.kind = DataKind::Passive;
                }
                wasmparser::DataKind::Active {
                    memory_index,
                    offset_expr,
                } => {
                    data.value = segment.data.to_vec();

                    let memory_id = ids.get_memory(memory_index)?;
                    let memory = self.memories.get_mut(memory_id);
                    memory.data_segments.insert(data.id);

                    let offset = ConstExpr::eval(&offset_expr, ids)
                        .with_context(|| format!("failed to evaluate the offset of data {}", i))?;

                    if memory.memory64 {
                        match offset {
                            ConstExpr::Value(Value::I64(_)) => {}
                            ConstExpr::Global(global)
                                if self.globals.get(global).ty == ValType::I64 => {}
                            _ => bail!(
                                "data {} is active for 64-bit memory but has non-i64 offset",
                                i
                            ),
                        }
                    } else {
                        match offset {
                            ConstExpr::Value(Value::I32(_)) => {}
                            ConstExpr::Global(global)
                                if self.globals.get(global).ty == ValType::I32 => {}
                            _ => bail!(
                                "data {} is active for 32-bit memory but has non-i32 offset",
                                i
                            ),
                        }
                    }
                    data.kind = DataKind::Active {
                        memory: memory_id,
                        offset,
                    }
                }
            }
        }
        Ok(())
    }
}

impl Emit for ModuleData {
    fn emit(&self, cx: &mut EmitContext) {
        log::debug!("emit data section");

        if self.arena.len() == 0 {
            return;
        }

        // The encodings here are with respect to the bulk memory proposal, but
        // should be backwards compatible with the current MVP WebAssembly spec
        // so long as the only memory 0 is used.
        let mut wasm_data_section = wasm_encoder::DataSection::new();
        for data in self.iter() {
            match data.kind {
                DataKind::Passive => {
                    wasm_data_section.passive(data.value.clone());
                }
                DataKind::Active { memory, offset } => {
                    wasm_data_section.active(
                        cx.indices.get_memory_index(memory),
                        &offset.to_wasmencoder_type(cx),
                        data.value.clone(),
                    );
                }
            }
        }
        cx.wasm_module.section(&wasm_data_section);
    }
}
