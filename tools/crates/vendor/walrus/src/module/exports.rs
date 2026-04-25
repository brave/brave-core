//! Exported items in a wasm module.

use anyhow::Context;

use crate::emit::{Emit, EmitContext};
use crate::parse::IndicesToIds;
use crate::tombstone_arena::{Id, Tombstone, TombstoneArena};
use crate::{FunctionId, GlobalId, MemoryId, Module, Result, TableId};

/// The id of an export.
pub type ExportId = Id<Export>;

/// A named item exported from the wasm.
#[derive(Clone, Debug)]
pub struct Export {
    id: ExportId,
    /// The name of this export.
    pub name: String,
    /// The item being exported.
    pub item: ExportItem,
}

impl Tombstone for Export {
    fn on_delete(&mut self) {
        self.name = String::new();
    }
}

impl Export {
    /// Returns the id of this export
    pub fn id(&self) -> ExportId {
        self.id
    }
}

/// An exported item.
#[derive(Copy, Clone, Debug)]
pub enum ExportItem {
    /// An exported function.
    Function(FunctionId),
    /// An exported table.
    Table(TableId),
    /// An exported memory.
    Memory(MemoryId),
    /// An exported global.
    Global(GlobalId),
}

/// The set of exports in a module.
#[derive(Debug, Default)]
pub struct ModuleExports {
    /// The arena containing this module's exports.
    arena: TombstoneArena<Export>,
}

impl ModuleExports {
    /// Gets a reference to an export given its id
    pub fn get(&self, id: ExportId) -> &Export {
        &self.arena[id]
    }

    /// Gets a reference to an export given its id
    pub fn get_mut(&mut self, id: ExportId) -> &mut Export {
        &mut self.arena[id]
    }

    /// Delete an export entry from this module.
    pub fn delete(&mut self, id: ExportId) {
        self.arena.delete(id);
    }

    /// Get a shared reference to this module's exports.
    pub fn iter(&self) -> impl Iterator<Item = &Export> {
        self.arena.iter().map(|(_, f)| f)
    }

    /// Get a mutable reference to this module's exports.
    pub fn iter_mut(&mut self) -> impl Iterator<Item = &mut Export> {
        self.arena.iter_mut().map(|(_, f)| f)
    }

    /// Add a new export to this module
    pub fn add(&mut self, name: &str, item: impl Into<ExportItem>) -> ExportId {
        self.arena.alloc_with_id(|id| Export {
            id,
            name: name.to_string(),
            item: item.into(),
        })
    }

    #[doc(hidden)]
    #[deprecated(note = "Use `ModuleExports::delete` instead")]
    pub fn remove_root(&mut self, id: ExportId) {
        self.delete(id);
    }

    /// Get a reference to a function export given its function id.
    pub fn get_exported_func(&self, f: FunctionId) -> Option<&Export> {
        self.iter().find(|e| match e.item {
            ExportItem::Function(f0) => f0 == f,
            _ => false,
        })
    }

    /// Retrieve an exported function by export name
    pub fn get_func(&self, name: impl AsRef<str>) -> Result<FunctionId> {
        self.iter()
            .find_map(|expt| match expt.item {
                ExportItem::Function(fid) if expt.name == name.as_ref() => Some(fid),
                _ => None,
            })
            .with_context(|| format!("unable to find function export '{}'", name.as_ref()))
    }

    /// Get a reference to a table export given its table id.
    pub fn get_exported_table(&self, t: TableId) -> Option<&Export> {
        self.iter().find(|e| match e.item {
            ExportItem::Table(t0) => t0 == t,
            _ => false,
        })
    }

    /// Get a reference to a memory export given its export id.
    pub fn get_exported_memory(&self, m: MemoryId) -> Option<&Export> {
        self.iter().find(|e| match e.item {
            ExportItem::Memory(m0) => m0 == m,
            _ => false,
        })
    }

    /// Get a reference to a global export given its global id.
    pub fn get_exported_global(&self, g: GlobalId) -> Option<&Export> {
        self.iter().find(|e| match e.item {
            ExportItem::Global(g0) => g0 == g,
            _ => false,
        })
    }

    /// Delete an export by name from this module.
    pub fn remove(&mut self, name: impl AsRef<str>) -> Result<()> {
        let export = self
            .iter()
            .find(|e| e.name == name.as_ref())
            .with_context(|| {
                format!("failed to find exported func with name [{}]", name.as_ref())
            })?;

        self.delete(export.id());

        Ok(())
    }
}

impl Module {
    /// Construct the export set for a wasm module.
    pub(crate) fn parse_exports(
        &mut self,
        section: wasmparser::ExportSectionReader,
        ids: &IndicesToIds,
    ) -> Result<()> {
        log::debug!("parse export section");
        use wasmparser::ExternalKind::*;

        for entry in section {
            let entry = entry?;
            let item = match entry.kind {
                Func => ExportItem::Function(ids.get_func(entry.index)?),
                Table => ExportItem::Table(ids.get_table(entry.index)?),
                Memory => ExportItem::Memory(ids.get_memory(entry.index)?),
                Global => ExportItem::Global(ids.get_global(entry.index)?),
                Tag => {
                    unimplemented!("exception handling not supported");
                }
            };
            self.exports.arena.alloc_with_id(|id| Export {
                id,
                name: entry.name.to_string(),
                item,
            });
        }
        Ok(())
    }
}

impl Emit for ModuleExports {
    fn emit(&self, cx: &mut EmitContext) {
        log::debug!("emit export section");
        let mut wasm_export_section = wasm_encoder::ExportSection::new();

        // NB: exports are always considered used. They are the roots that the
        // used analysis searches out from.

        let count = self.iter().count();
        if count == 0 {
            return;
        }

        for export in self.iter() {
            match export.item {
                ExportItem::Function(id) => {
                    let index = cx.indices.get_func_index(id);
                    wasm_export_section.export(&export.name, wasm_encoder::ExportKind::Func, index);
                }
                ExportItem::Table(id) => {
                    let index = cx.indices.get_table_index(id);
                    wasm_export_section.export(
                        &export.name,
                        wasm_encoder::ExportKind::Table,
                        index,
                    );
                }
                ExportItem::Memory(id) => {
                    let index = cx.indices.get_memory_index(id);
                    wasm_export_section.export(
                        &export.name,
                        wasm_encoder::ExportKind::Memory,
                        index,
                    );
                }
                ExportItem::Global(id) => {
                    let index = cx.indices.get_global_index(id);
                    wasm_export_section.export(
                        &export.name,
                        wasm_encoder::ExportKind::Global,
                        index,
                    );
                }
            }
        }

        cx.wasm_module.section(&wasm_export_section);
    }
}

impl From<MemoryId> for ExportItem {
    fn from(id: MemoryId) -> ExportItem {
        ExportItem::Memory(id)
    }
}

impl From<FunctionId> for ExportItem {
    fn from(id: FunctionId) -> ExportItem {
        ExportItem::Function(id)
    }
}

impl From<GlobalId> for ExportItem {
    fn from(id: GlobalId) -> ExportItem {
        ExportItem::Global(id)
    }
}

impl From<TableId> for ExportItem {
    fn from(id: TableId) -> ExportItem {
        ExportItem::Table(id)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{FunctionBuilder, Module};
    use id_arena::Arena;

    /// this function always returns the same ID
    fn always_the_same_id<T>() -> Id<T> {
        let arena: Arena<T> = Arena::new();
        arena.next_id()
    }

    #[test]
    fn get_exported_func() {
        let mut module = Module::default();
        let mut builder = FunctionBuilder::new(&mut module.types, &[], &[]);
        builder.func_body().i32_const(1234).drop();
        let id: FunctionId = builder.finish(vec![], &mut module.funcs);
        module.exports.add("dummy", id);

        let actual: Option<&Export> = module.exports.get_exported_func(id);

        let export: &Export = actual.expect("Expected Some(Export) got None");
        assert_eq!(export.name, "dummy");
        match export.item {
            ExportItem::Function(f) => assert_eq!(f, id),
            _ => panic!("Expected a Function variant"),
        }
    }

    #[test]
    fn get_exported_func_should_return_none_for_unknown_function_id() {
        let module = Module::default();
        let id: FunctionId = always_the_same_id();
        let actual: Option<&Export> = module.exports.get_exported_func(id);
        assert!(actual.is_none());
    }

    #[test]
    fn get_exported_table() {
        let mut module = Module::default();
        let id: TableId = always_the_same_id();
        module.exports.add("dummy", id);

        let actual: Option<&Export> = module.exports.get_exported_table(id);

        let export: &Export = actual.expect("Expected Some(Export) got None");
        assert_eq!(export.name, "dummy");
        match export.item {
            ExportItem::Table(f) => assert_eq!(f, id),
            _ => panic!("Expected a Table variant"),
        }
    }

    #[test]
    fn get_exported_table_should_return_none_for_unknown_table_id() {
        let module = Module::default();
        let id: TableId = always_the_same_id();
        let actual: Option<&Export> = module.exports.get_exported_table(id);
        assert!(actual.is_none());
    }

    #[test]
    fn get_exported_memory() {
        let mut module = Module::default();
        let id: MemoryId = always_the_same_id();
        module.exports.add("dummy", id);

        let actual: Option<&Export> = module.exports.get_exported_memory(id);

        let export: &Export = actual.expect("Expected Some(Export) got None");
        assert_eq!(export.name, "dummy");
        match export.item {
            ExportItem::Memory(f) => assert_eq!(f, id),
            _ => panic!("Expected a Memory variant"),
        }
    }

    #[test]
    fn get_exported_memory_should_return_none_for_unknown_memory_id() {
        let module = Module::default();
        let id: MemoryId = always_the_same_id();
        let actual: Option<&Export> = module.exports.get_exported_memory(id);
        assert!(actual.is_none());
    }

    #[test]
    fn get_exported_global() {
        let mut module = Module::default();
        let id: GlobalId = always_the_same_id();
        module.exports.add("dummy", id);

        let actual: Option<&Export> = module.exports.get_exported_global(id);

        let export: &Export = actual.expect("Expected Some(Export) got None");
        assert_eq!(export.name, "dummy");
        match export.item {
            ExportItem::Global(f) => assert_eq!(f, id),
            _ => panic!("Expected a Global variant"),
        }
    }

    #[test]
    fn get_exported_global_should_return_none_for_unknown_global_id() {
        let module = Module::default();
        let id: GlobalId = always_the_same_id();
        let actual: Option<&Export> = module.exports.get_exported_global(id);
        assert!(actual.is_none());
    }

    #[test]
    fn delete() {
        let mut module = Module::default();
        let fn_id: FunctionId = always_the_same_id();
        let export_id: ExportId = module.exports.add("dummy", fn_id);
        assert!(module.exports.get_exported_func(fn_id).is_some());
        module.exports.delete(export_id);

        assert!(module.exports.get_exported_func(fn_id).is_none());
    }

    #[test]
    fn get_func_by_name() {
        let mut module = Module::default();
        let fn_id: FunctionId = always_the_same_id();
        let export_id: ExportId = module.exports.add("dummy", fn_id);
        assert!(module.exports.get_func("dummy").is_ok());
        module.exports.delete(export_id);
        assert!(module.exports.get_func("dummy").is_err());
    }

    #[test]
    fn iter_mut_can_update_export_item() {
        let mut module = Module::default();

        let mut builder = FunctionBuilder::new(&mut module.types, &[], &[]);
        builder.func_body().i32_const(1234).drop();
        let fn_id0: FunctionId = builder.finish(vec![], &mut module.funcs);

        let mut builder = FunctionBuilder::new(&mut module.types, &[], &[]);
        builder.func_body().i32_const(1234).drop();
        let fn_id1: FunctionId = builder.finish(vec![], &mut module.funcs);

        assert_ne!(fn_id0, fn_id1);

        let export_id: ExportId = module.exports.add("dummy", fn_id0);
        assert!(module.exports.get_exported_func(fn_id0).is_some());

        module
            .exports
            .iter_mut()
            .for_each(|x: &mut Export| x.item = ExportItem::Function(fn_id1));

        assert!(module.exports.get_exported_func(fn_id0).is_none());
        let actual: &Export = module
            .exports
            .get_exported_func(fn_id1)
            .expect("Expected Some(Export) got None");
        assert_eq!(actual.id, export_id);
        assert_eq!(actual.name, "dummy");
        match actual.item {
            ExportItem::Function(f) => assert_eq!(f, fn_id1),
            _ => panic!("Expected a Function variant"),
        }
    }
}
