//! Globals within a wasm module.
use crate::emit::{Emit, EmitContext};
use crate::parse::IndicesToIds;
use crate::tombstone_arena::{Id, Tombstone, TombstoneArena};
use crate::{ConstExpr, ImportId, Module, Result, ValType};

/// The id of a global.
pub type GlobalId = Id<Global>;

/// A wasm global.
#[derive(Debug)]
pub struct Global {
    // NB: Not public so that it can't get out of sync with the arena this is
    // contained within.
    id: GlobalId,
    /// This global's type.
    pub ty: ValType,
    /// Whether this global is mutable or not.
    pub mutable: bool,
    /// Whether this global is shared or not.
    pub shared: bool,
    /// The kind of global this is
    pub kind: GlobalKind,
    /// The name of this data, used for debugging purposes in the `name`
    /// custom section.
    pub name: Option<String>,
}

impl Tombstone for Global {}

/// The different kinds of globals a wasm module can have
#[derive(Debug)]
pub enum GlobalKind {
    /// An imported global without a known initializer
    Import(ImportId),
    /// A locally declare global with the specified identifier
    Local(ConstExpr),
}

impl Global {
    /// Get this global's id.
    pub fn id(&self) -> GlobalId {
        self.id
    }
}

/// The set of globals in each function in this module.
#[derive(Debug, Default)]
pub struct ModuleGlobals {
    /// The arena where the globals are stored.
    arena: TombstoneArena<Global>,
}

impl ModuleGlobals {
    /// Adds a new imported global to this list.
    pub fn add_import(
        &mut self,
        ty: ValType,
        mutable: bool,
        shared: bool,
        import_id: ImportId,
    ) -> GlobalId {
        self.arena.alloc_with_id(|id| Global {
            id,
            ty,
            mutable,
            shared,
            kind: GlobalKind::Import(import_id),
            name: None,
        })
    }

    /// Construct a new global, that does not originate from any of the input
    /// wasm globals.
    pub fn add_local(
        &mut self,
        ty: ValType,
        mutable: bool,
        shared: bool,
        init: ConstExpr,
    ) -> GlobalId {
        self.arena.alloc_with_id(|id| Global {
            id,
            ty,
            mutable,
            shared,
            kind: GlobalKind::Local(init),
            name: None,
        })
    }

    /// Gets a reference to a global given its id
    pub fn get(&self, id: GlobalId) -> &Global {
        &self.arena[id]
    }

    /// Gets a reference to a global given its id
    pub fn get_mut(&mut self, id: GlobalId) -> &mut Global {
        &mut self.arena[id]
    }

    /// Removes a global from this module.
    ///
    /// It is up to you to ensure that any potential references to the deleted
    /// global are also removed, eg `get_global` expressions.
    pub fn delete(&mut self, id: GlobalId) {
        self.arena.delete(id);
    }

    /// Get a shared reference to this module's globals.
    pub fn iter(&self) -> impl Iterator<Item = &Global> {
        self.arena.iter().map(|(_, f)| f)
    }
}

impl Module {
    /// Construct a new, empty set of globals for a module.
    pub(crate) fn parse_globals(
        &mut self,
        section: wasmparser::GlobalSectionReader,
        ids: &mut IndicesToIds,
    ) -> Result<()> {
        log::debug!("parse global section");
        for g in section {
            let g = g?;
            let id = self.globals.add_local(
                ValType::parse(&g.ty.content_type)?,
                g.ty.mutable,
                g.ty.shared,
                ConstExpr::eval(&g.init_expr, ids)?,
            );
            ids.push_global(id);
        }
        Ok(())
    }
}

impl Emit for ModuleGlobals {
    fn emit(&self, cx: &mut EmitContext) {
        log::debug!("emit global section");
        let mut wasm_global_section = wasm_encoder::GlobalSection::new();

        fn get_local(global: &Global) -> Option<(&Global, &ConstExpr)> {
            match &global.kind {
                GlobalKind::Import(_) => None,
                GlobalKind::Local(local) => Some((global, local)),
            }
        }

        // All imported globals emitted earlier during the import section, so
        // filter those out.
        let globals = self.iter().filter_map(get_local).count();
        if globals == 0 {
            return;
        }

        for (global, local) in self.iter().filter_map(get_local) {
            cx.indices.push_global(global.id());

            wasm_global_section.global(
                wasm_encoder::GlobalType {
                    val_type: global.ty.to_wasmencoder_type(),
                    mutable: global.mutable,
                    shared: global.shared,
                },
                &local.to_wasmencoder_type(cx),
            );
        }

        cx.wasm_module.section(&wasm_global_section);
    }
}
