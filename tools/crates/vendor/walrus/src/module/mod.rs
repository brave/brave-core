//! A high-level API for manipulating wasm modules.

mod config;
mod custom;
mod data;
mod debug;
mod elements;
mod exports;
mod functions;
mod globals;
mod imports;
mod locals;
mod memories;
mod producers;
mod tables;
mod types;

use crate::emit::{Emit, EmitContext, IdsToIndices};
use crate::error::Result;
pub use crate::ir::InstrLocId;
pub use crate::module::custom::{
    CustomSection, CustomSectionId, ModuleCustomSections, RawCustomSection, TypedCustomSectionId,
    UntypedCustomSectionId,
};
pub use crate::module::data::{Data, DataId, DataKind, ModuleData};
pub use crate::module::debug::ModuleDebugData;
pub use crate::module::elements::{Element, ElementId, ModuleElements};
pub use crate::module::elements::{ElementItems, ElementKind};
pub use crate::module::exports::{Export, ExportId, ExportItem, ModuleExports};
pub use crate::module::functions::{FuncParams, FuncResults};
pub use crate::module::functions::{Function, FunctionId, ModuleFunctions};
pub use crate::module::functions::{FunctionKind, ImportedFunction, LocalFunction};
pub use crate::module::globals::{Global, GlobalId, GlobalKind, ModuleGlobals};
pub use crate::module::imports::{Import, ImportId, ImportKind, ModuleImports};
pub use crate::module::locals::ModuleLocals;
pub use crate::module::memories::{Memory, MemoryId, ModuleMemories};
pub use crate::module::producers::ModuleProducers;
pub use crate::module::tables::{ModuleTables, Table, TableId};
pub use crate::module::types::ModuleTypes;
use crate::parse::IndicesToIds;
use anyhow::{bail, Context};
use id_arena::Id;
use log::warn;
use std::fs;
use std::mem;
use std::ops::Range;
use std::path::Path;
use wasmparser::{BinaryReader, Parser, Payload, Validator};

pub use self::config::ModuleConfig;

/// A wasm module.
#[derive(Debug, Default)]
#[allow(missing_docs)]
pub struct Module {
    pub imports: ModuleImports,
    pub tables: ModuleTables,
    pub types: ModuleTypes,
    pub funcs: ModuleFunctions,
    pub globals: ModuleGlobals,
    pub locals: ModuleLocals,
    pub exports: ModuleExports,
    pub memories: ModuleMemories,
    /// Registration of passive data segments, if any
    pub data: ModuleData,
    /// Registration of passive element segments, if any
    pub elements: ModuleElements,
    /// The `start` function, if any
    pub start: Option<FunctionId>,
    /// Representation of the eventual custom section, `producers`
    pub producers: ModuleProducers,
    /// Custom sections found in this module.
    pub customs: ModuleCustomSections,
    /// Dwarf debug data.
    pub debug: ModuleDebugData,
    /// The name of this module, used for debugging purposes in the `name`
    /// custom section.
    pub name: Option<String>,
    pub(crate) config: ModuleConfig,
}

/// Code transformation records, which is used to transform DWARF debug entries.
#[derive(Debug, Default)]
pub struct CodeTransform {
    /// Maps from an offset of an instruction in the input Wasm to its offset in the
    /// output Wasm.
    ///
    /// Note that an input offset may be mapped to multiple output offsets, and vice
    /// versa, due to transformations like function inlinining or constant
    /// propagation.
    pub instruction_map: Vec<(InstrLocId, usize)>,

    /// Offset of code section from the front of Wasm binary
    pub code_section_start: usize,

    /// Emitted binary ranges of functions
    pub function_ranges: Vec<(Id<Function>, Range<usize>)>,
}

impl Module {
    /// Create a default, empty module that uses the given configuration.
    pub fn with_config(config: ModuleConfig) -> Self {
        Module {
            config,
            ..Default::default()
        }
    }

    /// Construct a new module from the given path with the default
    /// configuration.
    pub fn from_file<P>(path: P) -> Result<Module>
    where
        P: AsRef<Path>,
    {
        Module::from_buffer(&fs::read(path)?)
    }

    /// Construct a new module from the given path and configuration.
    pub fn from_file_with_config<P>(path: P, config: &ModuleConfig) -> Result<Module>
    where
        P: AsRef<Path>,
    {
        config.parse(&fs::read(path)?)
    }

    /// Construct a new module from the in-memory wasm buffer with the default
    /// configuration.
    pub fn from_buffer(wasm: &[u8]) -> Result<Module> {
        ModuleConfig::new().parse(wasm)
    }

    /// Construct a new module from the in-memory wasm buffer and configuration.
    pub fn from_buffer_with_config(wasm: &[u8], config: &ModuleConfig) -> Result<Module> {
        config.parse(wasm)
    }

    fn parse(wasm: &[u8], config: &ModuleConfig) -> Result<Module> {
        let mut ret = Module {
            config: config.clone(),
            ..Default::default()
        };
        let mut indices = IndicesToIds::default();

        // For now we have the same set of wasm features
        // regardless of config.only_stable_features. New unstable features
        // may be enabled under `only_stable_features: false` in future.
        let wasm_features = config.get_wasmparser_wasm_features();

        let mut validator = Validator::new_with_features(wasm_features);

        let mut local_functions = Vec::new();
        let mut debug_sections = Vec::new();

        let mut parser = Parser::new(0);
        parser.set_features(wasm_features);

        for payload in parser.parse_all(wasm) {
            match payload? {
                Payload::Version {
                    num,
                    encoding,
                    range,
                } => {
                    validator.version(num, encoding, &range)?;
                }
                Payload::DataSection(s) => {
                    validator
                        .data_section(&s)
                        .context("failed to parse data section")?;
                    ret.parse_data(s, &mut indices)?;
                }
                Payload::TypeSection(s) => {
                    validator
                        .type_section(&s)
                        .context("failed to parse type section")?;
                    ret.parse_types(s, &mut indices)?;
                }
                Payload::ImportSection(s) => {
                    validator
                        .import_section(&s)
                        .context("failed to parse import section")?;
                    ret.parse_imports(s, &mut indices)?;
                }
                Payload::TableSection(s) => {
                    validator
                        .table_section(&s)
                        .context("failed to parse table section")?;
                    ret.parse_tables(s, &mut indices)?;
                }
                Payload::MemorySection(s) => {
                    validator
                        .memory_section(&s)
                        .context("failed to parse memory section")?;
                    ret.parse_memories(s, &mut indices)?;
                }
                Payload::GlobalSection(s) => {
                    validator
                        .global_section(&s)
                        .context("failed to parse global section")?;
                    ret.parse_globals(s, &mut indices)?;
                }
                Payload::ExportSection(s) => {
                    validator
                        .export_section(&s)
                        .context("failed to parse export section")?;
                    ret.parse_exports(s, &indices)?;
                }
                Payload::ElementSection(s) => {
                    validator
                        .element_section(&s)
                        .context("failed to parse element section")?;
                    ret.parse_elements(s, &mut indices)?;
                }
                Payload::StartSection { func, range, .. } => {
                    validator.start_section(func, &range)?;
                    ret.start = Some(indices.get_func(func)?);
                }
                Payload::FunctionSection(s) => {
                    validator
                        .function_section(&s)
                        .context("failed to parse function section")?;
                    ret.declare_local_functions(s, &mut indices)?;
                }
                Payload::DataCountSection { count, range } => {
                    validator.data_count_section(count, &range)?;
                    ret.reserve_data(count, &mut indices);
                }
                Payload::CodeSectionStart { count, range, .. } => {
                    validator.code_section_start(count, &range)?;
                    ret.funcs.code_section_offset = range.start;
                }
                Payload::CodeSectionEntry(body) => {
                    let validator = validator
                        .code_section_entry(&body)?
                        .into_validator(Default::default());
                    local_functions.push((body, validator));
                }
                Payload::CustomSection(s) => {
                    let result =
                        match s.name() {
                            "producers" => wasmparser::ProducersSectionReader::new(
                                BinaryReader::new(s.data(), s.data_offset(), wasm_features),
                            )
                            .map_err(anyhow::Error::from)
                            .and_then(|s| ret.parse_producers_section(s)),
                            "name" => {
                                let name_section_reader = wasmparser::NameSectionReader::new(
                                    BinaryReader::new(s.data(), s.data_offset(), wasm_features),
                                );
                                ret.parse_name_section(name_section_reader, &indices)
                            }
                            name => {
                                log::debug!("parsing custom section `{}`", name);
                                if name.starts_with(".debug") {
                                    debug_sections.push(RawCustomSection {
                                        name: name.to_string(),
                                        data: s.data().to_vec(),
                                    });
                                } else {
                                    ret.customs.add(RawCustomSection {
                                        name: name.to_string(),
                                        data: s.data().to_vec(),
                                    });
                                }
                                continue;
                            }
                        };
                    if let Err(e) = result {
                        log::warn!("failed to parse `{}` custom section {}", s.name(), e);
                    }
                }
                Payload::UnknownSection { id, range, .. } => {
                    validator.unknown_section(id, &range)?;
                    unreachable!()
                }

                Payload::End(offset) => {
                    validator.end(offset)?;
                    continue;
                }

                // component module proposal is not implemented yet.
                Payload::ModuleSection { .. }
                | Payload::InstanceSection(..)
                | Payload::CoreTypeSection(..)
                | Payload::ComponentSection { .. }
                | Payload::ComponentInstanceSection(..)
                | Payload::ComponentAliasSection(..)
                | Payload::ComponentTypeSection(..)
                | Payload::ComponentCanonicalSection(..)
                | Payload::ComponentStartSection { .. }
                | Payload::ComponentImportSection(..)
                | Payload::ComponentExportSection(..) => {
                    bail!("not supported yet");
                }

                // exception handling is not implemented yet.
                Payload::TagSection(s) => {
                    validator.tag_section(&s)?;
                    bail!("not supported yet");
                }
            }
        }

        ret.parse_local_functions(
            local_functions,
            &mut indices,
            config.on_instr_loc.as_ref().map(|f| f.as_ref()),
        )
        .context("failed to parse code section")?;

        ret.parse_debug_sections(debug_sections)
            .context("failed to parse debug data section")?;

        ret.producers
            .add_processed_by("walrus", env!("CARGO_PKG_VERSION"));

        if let Some(on_parse) = &config.on_parse {
            on_parse(&mut ret, &indices)?;
        }

        log::debug!("parse complete");
        Ok(ret)
    }

    /// Emit this module into a `.wasm` file at the given path.
    pub fn emit_wasm_file<P>(&mut self, path: P) -> Result<()>
    where
        P: AsRef<Path>,
    {
        let buffer = self.emit_wasm();
        fs::write(path, buffer).context("failed to write wasm module")?;
        Ok(())
    }

    /// Emit this module into an in-memory wasm buffer.
    pub fn emit_wasm(&mut self) -> Vec<u8> {
        log::debug!("start emit");

        let indices = &mut IdsToIndices::default();

        let mut customs = mem::take(&mut self.customs);

        let mut cx = EmitContext {
            module: self,
            indices,
            wasm_module: wasm_encoder::Module::new(),
            locals: Default::default(),
            code_transform: Default::default(),
        };
        self.types.emit(&mut cx);
        self.imports.emit(&mut cx);
        self.funcs.emit_func_section(&mut cx);
        self.tables.emit(&mut cx);
        self.memories.emit(&mut cx);
        // TODO: tag section
        self.globals.emit(&mut cx);
        self.exports.emit(&mut cx);
        if let Some(start) = self.start {
            let idx = cx.indices.get_func_index(start);
            cx.wasm_module.section(&wasm_encoder::StartSection {
                function_index: idx,
            });
        }
        self.elements.emit(&mut cx);
        self.data.emit_data_count(&mut cx);
        self.funcs.emit(&mut cx);
        self.data.emit(&mut cx);

        if !self.config.skip_name_section {
            emit_name_section(&mut cx);
        }
        if !self.config.skip_producers_section {
            self.producers.emit(&mut cx);
        }

        if self.config.generate_dwarf {
            self.debug.emit(&mut cx);
        } else {
            log::debug!("skipping DWARF custom section");
        }

        let indices = std::mem::take(cx.indices);

        for (_id, section) in customs.iter_mut() {
            if section.name().starts_with(".debug") {
                continue;
            }

            log::debug!("emitting custom section {}", section.name());

            if self.config.preserve_code_transform {
                section.apply_code_transform(&cx.code_transform);
            }

            cx.wasm_module.section(&wasm_encoder::CustomSection {
                name: section.name().into(),
                data: section.data(&indices),
            });
        }

        let out = cx.wasm_module.finish();
        log::debug!("emission finished");

        // let mut validator = Validator::new();
        // if let Err(err) = validator.validate_all(&out) {
        //     eprintln!("{:?}", err);
        //     panic!("Unable to validate serialized output");
        // }

        out
    }

    /// Returns an iterator over all functions in this module
    pub fn functions(&self) -> impl Iterator<Item = &Function> {
        self.funcs.iter()
    }

    fn parse_name_section(
        &mut self,
        names: wasmparser::NameSectionReader,
        indices: &IndicesToIds,
    ) -> Result<()> {
        log::debug!("parse name section");
        for subsection in names {
            match subsection? {
                wasmparser::Name::Module {
                    name,
                    name_range: _,
                } => {
                    self.name = Some(name.to_string());
                }
                wasmparser::Name::Function(names) => {
                    for name in names {
                        let naming = name?;
                        match indices.get_func(naming.index) {
                            Ok(id) => self.funcs.get_mut(id).name = Some(naming.name.to_string()),
                            Err(e) => warn!("in name section: {}", e),
                        }
                    }
                }
                wasmparser::Name::Type(names) => {
                    for name in names {
                        let naming = name?;
                        match indices.get_type(naming.index) {
                            Ok(id) => self.types.get_mut(id).name = Some(naming.name.to_string()),
                            Err(e) => warn!("in name section: {}", e),
                        }
                    }
                }
                wasmparser::Name::Memory(names) => {
                    for name in names {
                        let naming = name?;
                        match indices.get_memory(naming.index) {
                            Ok(id) => {
                                self.memories.get_mut(id).name = Some(naming.name.to_string())
                            }
                            Err(e) => warn!("in name section: {}", e),
                        }
                    }
                }
                wasmparser::Name::Table(names) => {
                    for name in names {
                        let naming = name?;
                        match indices.get_table(naming.index) {
                            Ok(id) => self.tables.get_mut(id).name = Some(naming.name.to_string()),
                            Err(e) => warn!("in name section: {}", e),
                        }
                    }
                }
                wasmparser::Name::Data(names) => {
                    for name in names {
                        let naming = name?;
                        match indices.get_data(naming.index) {
                            Ok(id) => self.data.get_mut(id).name = Some(naming.name.to_string()),
                            Err(e) => warn!("in name section: {}", e),
                        }
                    }
                }
                wasmparser::Name::Element(names) => {
                    for name in names {
                        let naming = name?;
                        match indices.get_element(naming.index) {
                            Ok(id) => {
                                self.elements.get_mut(id).name = Some(naming.name.to_string())
                            }
                            Err(e) => warn!("in name section: {}", e),
                        }
                    }
                }
                wasmparser::Name::Global(names) => {
                    for name in names {
                        let naming = name?;
                        match indices.get_global(naming.index) {
                            Ok(id) => self.globals.get_mut(id).name = Some(naming.name.to_string()),
                            Err(e) => warn!("in name section: {}", e),
                        }
                    }
                }
                wasmparser::Name::Local(l) => {
                    for f in l {
                        let f = f?;
                        let func_id = indices.get_func(f.index)?;
                        for name in f.names {
                            let naming = name?;
                            // Looks like tools like `wat2wasm` generate empty
                            // names for locals if they aren't specified, so
                            // just ignore empty names which would in theory
                            // make debugging a bit harder.
                            if self.config.generate_synthetic_names_for_anonymous_items
                                && naming.name.is_empty()
                            {
                                continue;
                            }
                            match indices.get_local(func_id, naming.index) {
                                Ok(id) => {
                                    self.locals.get_mut(id).name = Some(naming.name.to_string())
                                }
                                // It looks like emscripten leaves broken
                                // function references in the locals subsection
                                // sometimes.
                                Err(e) => warn!("in name section: {}", e),
                            }
                        }
                    }
                }
                wasmparser::Name::Unknown { ty, .. } => warn!("unknown name subsection {}", ty),
                wasmparser::Name::Label(_) => warn!("labels name subsection ignored"),
                wasmparser::Name::Field(_) => warn!("fields name subsection ignored"),
                wasmparser::Name::Tag(_) => warn!("tags name subsection ignored"),
            }
        }
        Ok(())
    }
}

fn emit_name_section(cx: &mut EmitContext) {
    log::debug!("emit name section");

    let mut wasm_name_section = wasm_encoder::NameSection::new();

    let mut funcs = cx
        .module
        .funcs
        .iter()
        .filter_map(|func| func.name.as_ref().map(|name| (func, name)))
        .map(|(func, name)| (cx.indices.get_func_index(func.id()), name))
        .collect::<Vec<_>>();
    funcs.sort_by_key(|p| p.0); // sort by index

    let mut locals = cx
        .module
        .funcs
        .iter()
        .filter_map(|func| cx.locals.get(&func.id()).map(|l| (func, l)))
        .filter_map(|(func, locals)| {
            let local_names = locals
                .iter()
                .filter_map(|id| {
                    let name = cx.module.locals.get(*id).name.as_ref()?;
                    let index = cx.indices.locals.get(&func.id())?.get(id)?;
                    Some((*index, name))
                })
                .collect::<Vec<_>>();
            if local_names.is_empty() {
                None
            } else {
                Some((cx.indices.get_func_index(func.id()), local_names))
            }
        })
        .collect::<Vec<_>>();
    locals.sort_by_key(|p| p.0); // sort by index

    let mut types = cx
        .module
        .types
        .iter()
        .filter_map(|typ| typ.name.as_ref().map(|name| (typ, name)))
        .map(|(typ, name)| (cx.indices.get_type_index(typ.id()), name))
        .collect::<Vec<_>>();
    types.sort_by_key(|p| p.0); // sort by index

    let mut tables = cx
        .module
        .tables
        .iter()
        .filter_map(|table| table.name.as_ref().map(|name| (table, name)))
        .map(|(table, name)| (cx.indices.get_table_index(table.id()), name))
        .collect::<Vec<_>>();
    tables.sort_by_key(|p| p.0); // sort by index

    let mut memories = cx
        .module
        .memories
        .iter()
        .filter_map(|memory| memory.name.as_ref().map(|name| (memory, name)))
        .map(|(memory, name)| (cx.indices.get_memory_index(memory.id()), name))
        .collect::<Vec<_>>();
    memories.sort_by_key(|p| p.0); // sort by index

    let mut globals = cx
        .module
        .globals
        .iter()
        .filter_map(|global| global.name.as_ref().map(|name| (global, name)))
        .map(|(global, name)| (cx.indices.get_global_index(global.id()), name))
        .collect::<Vec<_>>();
    globals.sort_by_key(|p| p.0); // sort by index

    let mut elements = cx
        .module
        .elements
        .iter()
        .filter_map(|element| element.name.as_ref().map(|name| (element, name)))
        .map(|(element, name)| (cx.indices.get_element_index(element.id()), name))
        .collect::<Vec<_>>();
    elements.sort_by_key(|p| p.0); // sort by index

    let mut data = cx
        .module
        .data
        .iter()
        .filter_map(|data| data.name.as_ref().map(|name| (data, name)))
        .map(|(data, name)| (cx.indices.get_data_index(data.id()), name))
        .collect::<Vec<_>>();
    data.sort_by_key(|p| p.0); // sort by index

    if cx.module.name.is_none()
        && funcs.is_empty()
        && locals.is_empty()
        && types.is_empty()
        && tables.is_empty()
        && memories.is_empty()
        && globals.is_empty()
        && elements.is_empty()
        && data.is_empty()
    {
        return;
    }

    // Order of written subsections must match order defined in
    // `wasm_encorder::names::Subsection`.

    if let Some(name) = &cx.module.name {
        wasm_name_section.module(name);
    }

    if !funcs.is_empty() {
        let mut name_map = wasm_encoder::NameMap::new();
        for (index, name) in funcs {
            name_map.append(index, name);
        }
        wasm_name_section.functions(&name_map);
    }

    if !locals.is_empty() {
        let mut indirect_name_map = wasm_encoder::IndirectNameMap::new();
        for (index, mut map) in locals {
            let mut name_map = wasm_encoder::NameMap::new();
            map.sort_by_key(|p| p.0); // sort by index
            for (index, name) in map {
                name_map.append(index, name);
            }
            indirect_name_map.append(index, &name_map);
        }
        wasm_name_section.locals(&indirect_name_map);
    }

    if !types.is_empty() {
        let mut name_map = wasm_encoder::NameMap::new();
        for (index, name) in types {
            name_map.append(index, name);
        }
        wasm_name_section.types(&name_map);
    }

    if !tables.is_empty() {
        let mut name_map = wasm_encoder::NameMap::new();
        for (index, name) in tables {
            name_map.append(index, name);
        }
        wasm_name_section.tables(&name_map);
    }

    if !memories.is_empty() {
        let mut name_map = wasm_encoder::NameMap::new();
        for (index, name) in memories {
            name_map.append(index, name);
        }
        wasm_name_section.memories(&name_map);
    }

    if !globals.is_empty() {
        let mut name_map = wasm_encoder::NameMap::new();
        for (index, name) in globals {
            name_map.append(index, name);
        }
        wasm_name_section.globals(&name_map);
    }

    if !elements.is_empty() {
        let mut name_map = wasm_encoder::NameMap::new();
        for (index, name) in elements {
            name_map.append(index, name);
        }
        wasm_name_section.elements(&name_map);
    }

    if !data.is_empty() {
        let mut name_map = wasm_encoder::NameMap::new();
        for (index, name) in data {
            name_map.append(index, name);
        }
        wasm_name_section.data(&name_map);
    }

    cx.wasm_module.section(&wasm_name_section);
}
