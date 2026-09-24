//! Types in a wasm module.

use crate::arena_set::ArenaSet;
use crate::emit::{Emit, EmitContext};
use crate::error::Result;
use crate::module::Module;
use crate::parse::IndicesToIds;
use crate::ty::{Type, TypeId, ValType};

/// The set of de-duplicated types within a module.
#[derive(Debug, Default)]
pub struct ModuleTypes {
    arena: ArenaSet<Type>,
}

impl ModuleTypes {
    /// Get a type associated with an ID
    pub fn get(&self, id: TypeId) -> &Type {
        &self.arena[id]
    }

    /// Get a type associated with an ID
    pub fn get_mut(&mut self, id: TypeId) -> &mut Type {
        &mut self.arena[id]
    }

    /// Get the parameters and results for the given type.
    pub fn params_results(&self, id: TypeId) -> (&[ValType], &[ValType]) {
        let ty = self.get(id);
        (ty.params(), ty.results())
    }

    /// Get the parameters for the given type.
    pub fn params(&self, id: TypeId) -> &[ValType] {
        self.get(id).params()
    }

    /// Get the results for the given type.
    pub fn results(&self, id: TypeId) -> &[ValType] {
        self.get(id).results()
    }

    /// Get a type ID by its name.
    ///
    /// This is currently only intended for in-memory modifications, and by
    /// default will always return `None` for a newly parsed module. A
    /// hypothetical future WAT text format to `walrus::Module` parser could
    /// preserve type names from the WAT.
    pub fn by_name(&self, name: &str) -> Option<TypeId> {
        self.arena.iter().find_map(|(id, ty)| {
            if ty.name.as_deref() == Some(name) {
                Some(id)
            } else {
                None
            }
        })
    }

    /// Get a shared reference to this module's types.
    pub fn iter(&self) -> impl Iterator<Item = &Type> {
        self.arena.iter().map(|(_, f)| f)
    }

    /// Removes a type from this module.
    ///
    /// It is up to you to ensure that any potential references to the deleted
    /// type are also removed, eg `call_indirect` expressions, function types,
    /// etc.
    pub fn delete(&mut self, ty: TypeId) {
        self.arena.remove(ty);
    }

    /// Add a new type to this module, and return its `Id`
    pub fn add(&mut self, params: &[ValType], results: &[ValType]) -> TypeId {
        let id = self.arena.next_id();
        self.arena.insert(Type::new(
            id,
            params.to_vec().into_boxed_slice(),
            results.to_vec().into_boxed_slice(),
        ))
    }

    pub(crate) fn add_entry_ty(&mut self, results: &[ValType]) -> TypeId {
        let id = self.arena.next_id();
        self.arena.insert(Type::for_function_entry(
            id,
            results.to_vec().into_boxed_slice(),
        ))
    }

    /// Find the existing type for the given parameters and results.
    pub fn find(&self, params: &[ValType], results: &[ValType]) -> Option<TypeId> {
        self.arena.iter().find_map(|(id, ty)| {
            if !ty.is_for_function_entry() && ty.params() == params && ty.results() == results {
                Some(id)
            } else {
                None
            }
        })
    }

    pub(crate) fn find_for_function_entry(&self, results: &[ValType]) -> Option<TypeId> {
        self.arena.iter().find_map(|(id, ty)| {
            if ty.is_for_function_entry() && ty.params().is_empty() && ty.results() == results {
                Some(id)
            } else {
                None
            }
        })
    }
}

impl Module {
    /// Construct the set of types within a module.
    pub(crate) fn parse_types(
        &mut self,
        section: wasmparser::TypeSectionReader,
        ids: &mut IndicesToIds,
    ) -> Result<()> {
        log::debug!("parsing type section");
        for ty in section.into_iter_err_on_gc_types() {
            let fun_ty = ty?;
            let id = self.types.arena.next_id();
            let params = fun_ty
                .params()
                .iter()
                .map(ValType::parse)
                .collect::<Result<Vec<_>>>()?
                .into_boxed_slice();
            let results = fun_ty
                .results()
                .iter()
                .map(ValType::parse)
                .collect::<Result<Vec<_>>>()?
                .into_boxed_slice();
            let id = self.types.arena.insert(Type::new(id, params, results));
            ids.push_type(id);
        }

        Ok(())
    }
}

impl Emit for ModuleTypes {
    fn emit(&self, cx: &mut EmitContext) {
        log::debug!("emitting type section");

        let mut wasm_type_section = wasm_encoder::TypeSection::new();

        let mut tys = self
            .arena
            .iter()
            .filter(|(_, ty)| !ty.is_for_function_entry())
            .collect::<Vec<_>>();

        if tys.is_empty() {
            return;
        }

        // Sort for deterministic ordering.
        tys.sort_by_key(|&(_, ty)| ty);

        for (id, ty) in tys {
            cx.indices.push_type(id);
            wasm_type_section.function(
                ty.params().iter().map(ValType::to_wasmencoder_type),
                ty.results().iter().map(ValType::to_wasmencoder_type),
            );
        }

        cx.wasm_module.section(&wasm_type_section);
    }
}
