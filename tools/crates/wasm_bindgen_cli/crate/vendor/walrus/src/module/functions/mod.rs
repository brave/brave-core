//! Functions within a wasm module.

use std::cmp;
use std::collections::BTreeMap;
use std::ops::Range;

use anyhow::{bail, Context};
use wasm_encoder::Encode;
use wasmparser::{FuncValidator, FunctionBody, ValidatorResources};

#[cfg(feature = "parallel")]
use rayon::prelude::*;

mod local_function;

use crate::emit::{Emit, EmitContext};
use crate::error::Result;
use crate::ir::InstrLocId;
use crate::module::imports::ImportId;
use crate::module::Module;
use crate::parse::IndicesToIds;
use crate::tombstone_arena::{Id, Tombstone, TombstoneArena};
use crate::ty::TypeId;
use crate::ty::ValType;
use crate::{ExportItem, FunctionBuilder, InstrSeqBuilder, LocalId, Memory, MemoryId};

pub use self::local_function::LocalFunction;

/// A function identifier.
pub type FunctionId = Id<Function>;

/// Parameter(s) to a function
pub type FuncParams = Vec<ValType>;

/// Result(s) of a given function
pub type FuncResults = Vec<ValType>;

/// A wasm function.
///
/// Either defined locally or externally and then imported; see `FunctionKind`.
#[derive(Debug)]
pub struct Function {
    // NB: Not public so that it can't get out of sync with the arena that this
    // function lives within.
    id: FunctionId,

    /// The kind of function this is.
    pub kind: FunctionKind,

    /// An optional name associated with this function
    pub name: Option<String>,
}

impl Tombstone for Function {
    fn on_delete(&mut self) {
        let ty = self.ty();
        self.kind = FunctionKind::Uninitialized(ty);
        self.name = None;
    }
}

impl Function {
    fn new_uninitialized(id: FunctionId, ty: TypeId) -> Function {
        Function {
            id,
            kind: FunctionKind::Uninitialized(ty),
            name: None,
        }
    }

    /// Get this function's identifier.
    pub fn id(&self) -> FunctionId {
        self.id
    }

    /// Get this function's type's identifier.
    pub fn ty(&self) -> TypeId {
        match &self.kind {
            FunctionKind::Local(l) => l.ty(),
            FunctionKind::Import(i) => i.ty,
            FunctionKind::Uninitialized(t) => *t,
        }
    }
}

/// The local- or external-specific bits of a function.
#[derive(Debug)]
pub enum FunctionKind {
    /// An externally defined, imported wasm function.
    Import(ImportedFunction),

    /// A locally defined wasm function.
    Local(LocalFunction),

    /// A locally defined wasm function that we haven't parsed yet (but have
    /// reserved its id and associated it with its original input wasm module
    /// index). This should only exist within
    /// `ModuleFunctions::add_local_functions`.
    Uninitialized(TypeId),
}

impl FunctionKind {
    /// Get the underlying `FunctionKind::Import` or panic if this is not an
    /// import function
    pub fn unwrap_import(&self) -> &ImportedFunction {
        match self {
            FunctionKind::Import(import) => import,
            _ => panic!("not an import function"),
        }
    }

    /// Get the underlying `FunctionKind::Local` or panic if this is not a local
    /// function.
    pub fn unwrap_local(&self) -> &LocalFunction {
        match self {
            FunctionKind::Local(l) => l,
            _ => panic!("not a local function"),
        }
    }

    /// Get the underlying `FunctionKind::Import` or panic if this is not an
    /// import function
    pub fn unwrap_import_mut(&mut self) -> &mut ImportedFunction {
        match self {
            FunctionKind::Import(import) => import,
            _ => panic!("not an import function"),
        }
    }

    /// Get the underlying `FunctionKind::Local` or panic if this is not a local
    /// function.
    pub fn unwrap_local_mut(&mut self) -> &mut LocalFunction {
        match self {
            FunctionKind::Local(l) => l,
            _ => panic!("not a local function"),
        }
    }
}

/// An externally defined, imported function.
#[derive(Debug)]
pub struct ImportedFunction {
    /// The import that brings this function into the module.
    pub import: ImportId,
    /// The type signature of this imported function.
    pub ty: TypeId,
}

/// The set of functions within a module.
#[derive(Debug, Default)]
pub struct ModuleFunctions {
    /// The arena containing this module's functions.
    arena: TombstoneArena<Function>,

    /// Original code section offset.
    pub(crate) code_section_offset: usize,
}

impl ModuleFunctions {
    /// Construct a new, empty set of functions for a module.
    pub fn new() -> ModuleFunctions {
        Default::default()
    }

    /// Create a new externally defined, imported function.
    pub fn add_import(&mut self, ty: TypeId, import: ImportId) -> FunctionId {
        self.arena.alloc_with_id(|id| Function {
            id,
            kind: FunctionKind::Import(ImportedFunction { import, ty }),
            name: None,
        })
    }

    /// Create a new internally defined function
    pub fn add_local(&mut self, func: LocalFunction) -> FunctionId {
        let func_name = func.builder().name.clone();
        self.arena.alloc_with_id(|id| Function {
            id,
            kind: FunctionKind::Local(func),
            name: func_name,
        })
    }

    /// Gets a reference to a function given its id
    pub fn get(&self, id: FunctionId) -> &Function {
        &self.arena[id]
    }

    /// Gets a reference to a function given its id
    pub fn get_mut(&mut self, id: FunctionId) -> &mut Function {
        &mut self.arena[id]
    }

    /// Get a function ID by its name.
    ///
    /// The name used is the "name" custom section name and *not* the export
    /// name, if a function happens to be exported.
    ///
    /// Note that function names are *not* guaranteed to be unique. This will
    /// return the first function in the module with the given name.
    pub fn by_name(&self, name: &str) -> Option<FunctionId> {
        self.arena.iter().find_map(|(id, f)| {
            if f.name.as_deref() == Some(name) {
                Some(id)
            } else {
                None
            }
        })
    }

    /// Removes a function from this module.
    ///
    /// It is up to you to ensure that any potential references to the deleted
    /// function are also removed, eg `call` expressions, exports, table
    /// elements, etc.
    pub fn delete(&mut self, id: FunctionId) {
        self.arena.delete(id);
    }

    /// Get a shared reference to this module's functions.
    pub fn iter(&self) -> impl Iterator<Item = &Function> {
        self.arena.iter().map(|(_, f)| f)
    }

    /// Get a shared reference to this module's functions.
    ///
    /// Requires the `parallel` feature of this crate to be enabled.
    #[cfg(feature = "parallel")]
    pub fn par_iter(&self) -> impl ParallelIterator<Item = &Function> {
        self.arena.par_iter().map(|(_, f)| f)
    }

    /// Get an iterator of this module's local functions
    pub fn iter_local(&self) -> impl Iterator<Item = (FunctionId, &LocalFunction)> {
        self.iter().filter_map(|f| match &f.kind {
            FunctionKind::Local(local) => Some((f.id(), local)),
            _ => None,
        })
    }

    /// Get a parallel iterator of this module's local functions
    ///
    /// Requires the `parallel` feature of this crate to be enabled.
    #[cfg(feature = "parallel")]
    pub fn par_iter_local(&self) -> impl ParallelIterator<Item = (FunctionId, &LocalFunction)> {
        self.par_iter().filter_map(|f| match &f.kind {
            FunctionKind::Local(local) => Some((f.id(), local)),
            _ => None,
        })
    }

    /// Get a mutable reference to this module's functions.
    pub fn iter_mut(&mut self) -> impl Iterator<Item = &mut Function> {
        self.arena.iter_mut().map(|(_, f)| f)
    }

    /// Get a mutable reference to this module's functions.
    ///
    /// Requires the `parallel` feature of this crate to be enabled.
    #[cfg(feature = "parallel")]
    pub fn par_iter_mut(&mut self) -> impl ParallelIterator<Item = &mut Function> {
        self.arena.par_iter_mut().map(|(_, f)| f)
    }

    /// Get an iterator of this module's local functions
    pub fn iter_local_mut(&mut self) -> impl Iterator<Item = (FunctionId, &mut LocalFunction)> {
        self.iter_mut().filter_map(|f| {
            let id = f.id();
            match &mut f.kind {
                FunctionKind::Local(local) => Some((id, local)),
                _ => None,
            }
        })
    }

    /// Get a parallel iterator of this module's local functions
    ///
    /// Requires the `parallel` feature of this crate to be enabled.
    #[cfg(feature = "parallel")]
    pub fn par_iter_local_mut(
        &mut self,
    ) -> impl ParallelIterator<Item = (FunctionId, &mut LocalFunction)> {
        self.par_iter_mut().filter_map(|f| {
            let id = f.id();
            match &mut f.kind {
                FunctionKind::Local(local) => Some((id, local)),
                _ => None,
            }
        })
    }

    pub(crate) fn emit_func_section(&self, cx: &mut EmitContext) {
        log::debug!("emit function section");
        let functions = used_local_functions(cx);
        if functions.is_empty() {
            return;
        }
        let mut func_section = wasm_encoder::FunctionSection::new();
        for (id, function, _size) in functions {
            let index = cx.indices.get_type_index(function.ty());
            func_section.function(index);

            // Assign an index to all local defined functions before we start
            // translating them. While translating they may refer to future
            // functions, so we'll need to have an index for it by that point.
            // We're guaranteed the function section is emitted before the code
            // section so we should be covered here.
            cx.indices.push_func(id);
        }
        cx.wasm_module.section(&func_section);
    }
}

impl Module {
    /// Declare local functions after seeing the `function` section of a wasm
    /// executable.
    pub(crate) fn declare_local_functions(
        &mut self,
        section: wasmparser::FunctionSectionReader,
        ids: &mut IndicesToIds,
    ) -> Result<()> {
        log::debug!("parse function section");
        for func in section {
            let ty = ids.get_type(func?)?;
            let id = self
                .funcs
                .arena
                .alloc_with_id(|id| Function::new_uninitialized(id, ty));
            let idx = ids.push_func(id);
            if self.config.generate_synthetic_names_for_anonymous_items {
                self.funcs.get_mut(id).name = Some(format!("f{}", idx));
            }
        }

        Ok(())
    }

    /// Add the locally defined functions in the wasm module to this instance.
    pub(crate) fn parse_local_functions(
        &mut self,
        functions: Vec<(FunctionBody<'_>, FuncValidator<ValidatorResources>)>,
        indices: &mut IndicesToIds,
        on_instr_pos: Option<&(dyn Fn(&usize) -> InstrLocId + Sync + Send + 'static)>,
    ) -> Result<()> {
        log::debug!("parse code section");
        let num_imports = self.funcs.arena.len() - functions.len();

        // First up serially create corresponding `LocalId` instances for all
        // functions as well as extract the operators parser for each function.
        // This is pretty tough to parallelize, but we can look into it later if
        // necessary and it's a bottleneck!
        let mut bodies = Vec::with_capacity(functions.len());
        for (i, (body, mut validator)) in functions.into_iter().enumerate() {
            let index = (num_imports + i) as u32;
            let id = indices.get_func(index)?;
            let ty = match self.funcs.arena[id].kind {
                FunctionKind::Uninitialized(ty) => ty,
                _ => unreachable!(),
            };

            // First up, implicitly add locals for all function arguments. We also
            // record these in the function itself for later processing.
            let mut args = Vec::new();
            let type_ = self.types.get(ty);
            for ty in type_.params().iter() {
                let local_id = self.locals.add(*ty);
                let idx = indices.push_local(id, local_id);
                args.push(local_id);
                if self.config.generate_synthetic_names_for_anonymous_items {
                    let name = format!("arg{}", idx);
                    self.locals.get_mut(local_id).name = Some(name);
                }
            }

            // Ensure that there exists a `Type` for the function's entry
            // block. This is required because multi-value blocks reference a
            // `Type`, however function entry's type is implicit in the
            // encoding, and doesn't already exist in the `ModuleTypes`.
            let results = type_.results().to_vec();
            self.types.add_entry_ty(&results);

            // Next up comes all the locals of the function.
            let mut reader = body.get_binary_reader();
            for _ in 0..reader.read_var_u32()? {
                let pos = reader.original_position();
                let count = reader.read_var_u32()?;
                let ty = reader.read()?;
                validator.define_locals(pos, count, ty)?;
                let ty = ValType::parse(&ty)?;
                for _ in 0..count {
                    let local_id = self.locals.add(ty);
                    let idx = indices.push_local(id, local_id);
                    if self.config.generate_synthetic_names_for_anonymous_items {
                        let name = format!("l{}", idx);
                        self.locals.get_mut(local_id).name = Some(name);
                    }
                }
            }

            bodies.push((id, reader, args, ty, validator));
        }

        // Wasm modules can often have a lot of functions and this operation can
        // take some time, so parse all function bodies in parallel.
        let results = maybe_parallel!(bodies.(into_iter | into_par_iter))
            .map(|(id, body, args, ty, validator)| {
                (
                    id,
                    LocalFunction::parse(
                        self,
                        indices,
                        id,
                        ty,
                        args,
                        body,
                        on_instr_pos,
                        validator,
                    ),
                )
            })
            .collect::<Vec<_>>();

        // After all the function bodies are collected and finished push them
        // into our function arena.
        for (id, func) in results {
            let func = func?;
            self.funcs.arena[id].kind = FunctionKind::Local(func);
        }

        Ok(())
    }

    /// Retrieve the ID for the first exported memory.
    ///
    /// This method does not work in contexts with [multi-memory enabled](https://github.com/WebAssembly/multi-memory),
    /// and will error if more than one memory is present.
    pub fn get_memory_id(&self) -> Result<MemoryId> {
        if self.memories.len() > 1 {
            bail!("multiple memories unsupported")
        }

        self.memories
            .iter()
            .next()
            .map(Memory::id)
            .context("module does not export a memory")
    }

    /// Replace a single exported function with the result of the provided builder function.
    ///
    /// The builder function is provided a mutable reference to an [`InstrSeqBuilder`] which can be
    /// used to build the function as necessary.
    ///
    /// For example, if you wanted to replace an exported function with a no-op,
    ///
    /// ```ignore
    /// module.replace_exported_func(fid, |(body, arg_locals)| {
    ///     builder.func_body().unreachable();
    /// });
    /// ```
    ///
    /// The arguments passed to the original function will be passed to the
    /// new exported function that was built in your closure.
    ///
    /// This function returns the function ID of the *new* function,
    /// after it has been inserted into the module as an export.
    pub fn replace_exported_func(
        &mut self,
        fid: FunctionId,
        builder_fn: impl FnOnce((&mut InstrSeqBuilder, &Vec<LocalId>)),
    ) -> Result<FunctionId> {
        let original_export_id = self
            .exports
            .get_exported_func(fid)
            .map(|e| e.id())
            .with_context(|| format!("no exported function with ID [{fid:?}]"))?;

        if let Function {
            kind: FunctionKind::Local(lf),
            ..
        } = self.funcs.get(fid)
        {
            // Retrieve the params & result types for the exported (local) function
            let ty = self.types.get(lf.ty());
            let (params, results) = (ty.params().to_vec(), ty.results().to_vec());

            // Add the function produced by `fn_builder` as a local function
            let mut builder = FunctionBuilder::new(&mut self.types, &params, &results);
            let mut new_fn_body = builder.func_body();
            builder_fn((&mut new_fn_body, &lf.args));
            let func = builder.local_func(lf.args.clone());
            let new_fn_id = self.funcs.add_local(func);

            // Mutate the existing export to use the new local function
            let export = self.exports.get_mut(original_export_id);
            export.item = ExportItem::Function(new_fn_id);
            Ok(new_fn_id)
        } else {
            bail!("cannot replace function [{fid:?}], it is not an exported function");
        }
    }

    /// Replace a single imported function with the result of the provided builder function.
    ///
    /// The builder function is provided a mutable reference to an [`InstrSeqBuilder`] which can be
    /// used to build the function as necessary.
    ///
    /// For example, if you wanted to replace an imported function with a no-op,
    ///
    /// ```ignore
    /// module.replace_imported_func(fid, |(body, arg_locals)| {
    ///     builder.func_body().unreachable();
    /// });
    /// ```
    ///
    /// The arguments passed to the original function will be passed to the
    /// new exported function that was built in your closure.
    ///
    /// This function returns the function ID of the *new* function, and
    /// removes the existing import that has been replaced (the function will become local).
    pub fn replace_imported_func(
        &mut self,
        fid: FunctionId,
        builder_fn: impl FnOnce((&mut InstrSeqBuilder, &Vec<LocalId>)),
    ) -> Result<FunctionId> {
        let original_import_id = self
            .imports
            .get_imported_func(fid)
            .map(|import| import.id())
            .with_context(|| format!("no exported function with ID [{fid:?}]"))?;

        if let Function {
            kind: FunctionKind::Import(ImportedFunction { ty: tid, .. }),
            ..
        } = self.funcs.get(fid)
        {
            // Retrieve the params & result types for the imported function
            let ty = self.types.get(*tid);
            let (params, results) = (ty.params().to_vec(), ty.results().to_vec());

            // Build the list LocalIds used by args to match the original function
            let args = params
                .iter()
                .map(|ty| self.locals.add(*ty))
                .collect::<Vec<_>>();

            // Build the new function
            let mut builder = FunctionBuilder::new(&mut self.types, &params, &results);
            let mut new_fn_body = builder.func_body();
            builder_fn((&mut new_fn_body, &args));
            let new_func_kind = FunctionKind::Local(builder.local_func(args));

            // Mutate the existing function, changing it from a FunctionKind::ImportedFunction
            // to the local function produced by running the provided `fn_builder`
            let func = self.funcs.get_mut(fid);
            func.kind = new_func_kind;

            self.imports.delete(original_import_id);

            Ok(fid)
        } else {
            bail!("cannot replace function [{fid:?}], it is not an imported function");
        }
    }
}

fn used_local_functions<'a>(cx: &mut EmitContext<'a>) -> Vec<(FunctionId, &'a LocalFunction, u64)> {
    // Extract all local functions because imported ones were already
    // emitted as part of the import sectin. Find the size of each local
    // function. Sort imported functions in order so that we can get their
    // index in the function index space.
    let mut functions = Vec::new();
    for f in cx.module.funcs.iter() {
        match &f.kind {
            FunctionKind::Local(l) => functions.push((f.id(), l, l.size())),
            FunctionKind::Import(_) => {}
            FunctionKind::Uninitialized(_) => unreachable!(),
        }
    }

    // Sort local functions from largest to smallest; we will emit them in
    // this order. This helps load times, since wasm engines generally use
    // the function as their level of granularity for parallelism. We want
    // larger functions compiled before smaller ones because they will take
    // longer to compile.
    functions.sort_by_key(|(id, _, size)| (cmp::Reverse(*size), *id));

    functions
}

fn collect_non_default_code_offsets(
    code_transform: &mut BTreeMap<InstrLocId, usize>,
    code_offset: usize,
    map: Vec<(InstrLocId, usize)>,
) {
    for (src, dst) in map {
        let dst = dst + code_offset;
        if !src.is_default() {
            code_transform.insert(src, dst);
        }
    }
}

impl Emit for ModuleFunctions {
    fn emit(&self, cx: &mut EmitContext) {
        log::debug!("emit code section");
        let functions = used_local_functions(cx);
        if functions.is_empty() {
            return;
        }

        let mut wasm_code_section = wasm_encoder::CodeSection::new();
        let generate_map = cx.module.config.preserve_code_transform;

        // Functions can typically take awhile to serialize, so serialize
        // everything in parallel. Afterwards we'll actually place all the
        // functions together.
        let bytes = maybe_parallel!(functions.(into_iter | into_par_iter))
            .map(|(id, func, _size)| {
                log::debug!("emit function {:?} {:?}", id, cx.module.funcs.get(id).name);
                let mut wasm = Vec::new();
                let mut map = if generate_map { Some(Vec::new()) } else { None };

                let (locals_types, used_locals, local_indices) = func.emit_locals(cx.module);
                let mut wasm_function = wasm_encoder::Function::new(locals_types);
                func.emit_instructions(
                    cx.indices,
                    &local_indices,
                    &mut wasm_function,
                    map.as_mut(),
                );
                wasm_function.encode(&mut wasm);
                (
                    wasm,
                    wasm_function.byte_len(),
                    id,
                    used_locals,
                    local_indices,
                    map,
                )
            })
            .collect::<Vec<_>>();

        let mut instruction_map = BTreeMap::new();
        cx.indices.locals.reserve(bytes.len());

        let mut offset_data = Vec::new();
        for (wasm, byte_len, id, used_locals, local_indices, map) in bytes {
            let leb_len = wasm.len() - byte_len;
            wasm_code_section.raw(&wasm[leb_len..]);
            cx.indices.locals.insert(id, local_indices);
            cx.locals.insert(id, used_locals);
            offset_data.push((byte_len, id, map, leb_len));
        }
        cx.wasm_module.section(&wasm_code_section);

        let code_section_start_offset =
            cx.wasm_module.as_slice().len() - wasm_code_section.byte_len();
        let mut cur_offset = code_section_start_offset;

        // update the map afterwards based on final offset differences
        for (byte_len, id, map, leb_len) in offset_data {
            // (this assumes the leb encodes the same)
            let code_start_offset = cur_offset + leb_len;
            cur_offset += leb_len + byte_len;
            if let Some(map) = map {
                collect_non_default_code_offsets(&mut instruction_map, code_start_offset, map);
            }
            cx.code_transform.function_ranges.push((
                id,
                Range {
                    // inclusive leb part
                    start: code_start_offset - leb_len,
                    end: cur_offset,
                },
            ));
        }
        cx.code_transform.function_ranges.sort_by_key(|i| i.0);
        // FIXME: code section start in DWARF debug information expects 2 bytes before actual code section start.
        cx.code_transform.code_section_start = code_section_start_offset - 2;
        cx.code_transform.instruction_map = instruction_map.into_iter().collect();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Export, FunctionBuilder, Module};

    #[test]
    fn get_memory_id() {
        let mut module = Module::default();
        let expected_id = module.memories.add_local(false, false, 0, None, None);
        assert!(module.get_memory_id().is_ok_and(|id| id == expected_id));
    }

    /// Running `replace_exported_func` with a closure that builds
    /// a function should replace the existing function with the new one
    #[test]
    fn replace_exported_func() {
        let mut module = Module::default();

        // Create original function
        let mut builder = FunctionBuilder::new(&mut module.types, &[], &[]);
        builder.func_body().i32_const(1234).drop();
        let original_fn_id: FunctionId = builder.finish(vec![], &mut module.funcs);
        let original_export_id = module.exports.add("dummy", original_fn_id);

        // Replace the existing function with a new one with a reversed const value
        let new_fn_id = module
            .replace_exported_func(original_fn_id, |(body, _)| {
                body.i32_const(4321).drop();
            })
            .expect("function replacement worked");

        assert!(
            module.exports.get_exported_func(original_fn_id).is_none(),
            "replaced function cannot be gotten by ID"
        );

        // Ensure the function was replaced
        match module
            .exports
            .get_exported_func(new_fn_id)
            .expect("failed to unwrap exported func")
        {
            exp @ Export {
                item: ExportItem::Function(fid),
                ..
            } => {
                assert_eq!(*fid, new_fn_id, "retrieved function ID matches");
                assert_eq!(exp.id(), original_export_id, "export ID is unchanged");
            }
            _ => panic!("expected an Export with a Function inside"),
        }
    }

    /// Running `replace_exported_func` with a closure that returns None
    /// should replace the function with a generated no-op function
    #[test]
    fn replace_exported_func_generated_no_op() {
        let mut module = Module::default();

        // Create original function
        let mut builder = FunctionBuilder::new(&mut module.types, &[], &[]);
        builder.func_body().i32_const(1234).drop();
        let original_fn_id: FunctionId = builder.finish(vec![], &mut module.funcs);
        let original_export_id = module.exports.add("dummy", original_fn_id);

        // Replace the existing function with a new one with a reversed const value
        let new_fn_id = module
            .replace_exported_func(original_fn_id, |(body, _arg_locals)| {
                body.unreachable();
            })
            .expect("export function replacement worked");

        assert!(
            module.exports.get_exported_func(original_fn_id).is_none(),
            "replaced export function cannot be gotten by ID"
        );

        // Ensure the function was replaced
        match module
            .exports
            .get_exported_func(new_fn_id)
            .expect("failed to unwrap exported func")
        {
            exp @ Export {
                item: ExportItem::Function(fid),
                name,
                ..
            } => {
                assert_eq!(name, "dummy", "function name on export is unchanged");
                assert_eq!(*fid, new_fn_id, "retrieved function ID matches");
                assert_eq!(exp.id(), original_export_id, "export ID is unchanged");
            }
            _ => panic!("expected an Export with a Function inside"),
        }
    }

    /// Running `replace_imported_func` with a closure that builds
    /// a function should replace the existing function with the new one
    #[test]
    fn replace_imported_func() {
        let mut module = Module::default();

        // Create original import function
        let types = module.types.add(&[], &[]);
        let (original_fn_id, original_import_id) = module.add_import_func("mod", "dummy", types);

        // Replace the existing function with a new one with a reversed const value
        let new_fn_id = module
            .replace_imported_func(original_fn_id, |(body, _)| {
                body.i32_const(4321).drop();
            })
            .expect("import fn replacement worked");

        assert!(
            !module.imports.iter().any(|i| i.id() == original_import_id),
            "original import is missing",
        );

        assert!(
            module.imports.get_imported_func(original_fn_id).is_none(),
            "replaced import function cannot be gotten by ID"
        );

        assert!(
            module.imports.get_imported_func(new_fn_id).is_none(),
            "new import function cannot be gotten by ID (it is now local)"
        );

        assert!(
            matches!(module.funcs.get(new_fn_id).kind, FunctionKind::Local(_)),
            "new local function has the right kind"
        );
    }

    /// Running `replace_imported_func` with a closure that returns None
    /// should replace the function with a generated no-op function
    #[test]
    fn replace_imported_func_generated_no_op() {
        let mut module = Module::default();

        // Create original import function
        let types = module.types.add(&[], &[]);
        let (original_fn_id, original_import_id) = module.add_import_func("mod", "dummy", types);

        // Replace the existing function with a new one with a reversed const value
        let new_fn_id = module
            .replace_imported_func(original_fn_id, |(body, _arg_locals)| {
                body.unreachable();
            })
            .expect("import fn replacement worked");

        assert!(
            !module.imports.iter().any(|i| i.id() == original_import_id),
            "original import is missing",
        );

        assert!(
            module.imports.get_imported_func(original_fn_id).is_none(),
            "replaced import function cannot be gotten by ID"
        );

        assert!(
            module.imports.get_imported_func(new_fn_id).is_none(),
            "new import function cannot be gotten by ID (it is now local)"
        );

        assert!(
            matches!(module.funcs.get(new_fn_id).kind, FunctionKind::Local(_)),
            "new local function has the right kind"
        );
    }
}
