//! A crate to convert a WebAssembly binary to its textual representation in the
//! WebAssembly Text Format (WAT).
//!
//! This crate is intended for developer toolchains and debugging, supporting
//! human-readable versions of a wasm binary. This can also be useful when
//! developing wasm toolchain support in Rust for various purposes like testing
//! and debugging and such.

#![deny(missing_docs)]

use anyhow::{anyhow, bail, Context, Result};
use std::collections::{HashMap, HashSet};
use std::fmt;
use std::io;
use std::marker;
use std::mem;
use std::path::Path;
use wasmparser::*;

const MAX_LOCALS: u32 = 50000;
const MAX_NESTING_TO_PRINT: u32 = 50;
const MAX_WASM_FUNCTIONS: u32 = 1_000_000;
const MAX_WASM_FUNCTION_SIZE: u32 = 128 * 1024;

mod operator;
mod print;

pub use self::print::*;

/// Reads a WebAssembly `file` from the filesystem and then prints it into an
/// in-memory `String`.
pub fn print_file(file: impl AsRef<Path>) -> Result<String> {
    let file = file.as_ref();
    let contents = std::fs::read(file).context(format!("failed to read `{}`", file.display()))?;
    print_bytes(contents)
}

/// Prints an in-memory `wasm` binary blob into an in-memory `String` which is
/// its textual representation.
pub fn print_bytes(wasm: impl AsRef<[u8]>) -> Result<String> {
    let mut dst = String::new();
    Config::new().print(wasm.as_ref(), &mut PrintFmtWrite(&mut dst))?;
    Ok(dst)
}

/// Configuration used to print a WebAssembly binary.
///
/// This structure is used to control the overal structure of how wasm binaries
/// are printed and tweaks various ways that configures the output.
#[derive(Default)]
pub struct Config {
    print_offsets: bool,
    print_skeleton: bool,
    name_unnamed: bool,
}

/// This structure is the actual structure that prints WebAssembly binaries.
struct Printer<'cfg, 'env> {
    config: &'cfg Config,
    result: &'cfg mut (dyn Print + 'env),
    nesting: u32,
    line: usize,
    group_lines: Vec<usize>,
    code_section_hints: Vec<(u32, Vec<(usize, BranchHint)>)>,
}

#[derive(Default)]
struct CoreState {
    types: Vec<Option<SubType>>,
    funcs: u32,
    memories: u32,
    tags: u32,
    globals: u32,
    tables: u32,
    modules: u32,
    instances: u32,
    func_names: NamingMap<u32, NameFunc>,
    local_names: NamingMap<(u32, u32), NameLocal>,
    label_names: NamingMap<(u32, u32), NameLabel>,
    type_names: NamingMap<u32, NameType>,
    field_names: NamingMap<(u32, u32), NameField>,
    tag_names: NamingMap<u32, NameTag>,
    table_names: NamingMap<u32, NameTable>,
    memory_names: NamingMap<u32, NameMemory>,
    global_names: NamingMap<u32, NameGlobal>,
    element_names: NamingMap<u32, NameElem>,
    data_names: NamingMap<u32, NameData>,
    module_names: NamingMap<u32, NameModule>,
    instance_names: NamingMap<u32, NameInstance>,
}

/// A map of index-to-name for tracking what are the contents of the name
/// section.
///
/// The type parameter `T` is either `u32` for most index-based maps or a `(u32,
/// u32)` for label/local maps where there are two levels of indices.
///
/// The type parameter `K` is a static description/namespace for what kind of
/// item is contained within this map. That's used by some helper methods to
/// synthesize reasonable names automatically.
struct NamingMap<T, K> {
    index_to_name: HashMap<T, Naming>,
    _marker: marker::PhantomData<K>,
}

impl<T, K> Default for NamingMap<T, K> {
    fn default() -> NamingMap<T, K> {
        NamingMap {
            index_to_name: HashMap::new(),
            _marker: marker::PhantomData,
        }
    }
}

#[derive(Default)]
struct ComponentState {
    types: u32,
    funcs: u32,
    instances: u32,
    components: u32,
    values: u32,
    type_names: NamingMap<u32, NameType>,
    func_names: NamingMap<u32, NameFunc>,
    component_names: NamingMap<u32, NameComponent>,
    instance_names: NamingMap<u32, NameInstance>,
    value_names: NamingMap<u32, NameValue>,
}

struct State {
    encoding: Encoding,
    name: Option<Naming>,
    core: CoreState,
    component: ComponentState,
    custom_section_place: Option<&'static str>,
}

impl State {
    fn new(encoding: Encoding) -> Self {
        Self {
            encoding,
            name: None,
            core: CoreState::default(),
            component: ComponentState::default(),
            custom_section_place: None,
        }
    }
}

struct Naming {
    name: String,
    kind: NamingKind,
}

enum NamingKind {
    DollarName,
    DollarQuotedName,
    SyntheticPrefix(String),
}

impl Config {
    /// Creates a new [`Config`] object that's ready to start printing wasm
    /// binaries to strings.
    pub fn new() -> Self {
        Self::default()
    }

    /// Whether or not to print binary offsets of each item as comments in the
    /// text format whenever a newline is printed.
    pub fn print_offsets(&mut self, print: bool) {
        self.print_offsets = print;
    }

    /// Whether or not to print only a "skeleton" which skips function bodies,
    /// data segment contents, element segment contents, etc.
    pub fn print_skeleton(&mut self, print: bool) {
        self.print_skeleton = print;
    }

    /// Assign names to all unnamed items.
    ///
    /// If enabled then any previously unnamed item will have a name synthesized
    /// that looks like `$#func10` for example. The leading `#` indicates that
    /// it's `wasmprinter`-generated. The `func` is the namespace of the name
    /// and provides extra context about the item when referenced. The 10 is the
    /// local index of the item.
    ///
    /// Note that if the resulting text output is converted back to binary the
    /// resulting `name` custom section will not be the same as before.
    pub fn name_unnamed(&mut self, enable: bool) {
        self.name_unnamed = enable;
    }

    /// Prints a WebAssembly binary into a `String`
    ///
    /// This function takes an entire `wasm` binary blob and will print it to
    /// the WebAssembly Text Format and return the result as a `String`.
    pub fn print(&self, wasm: &[u8], result: &mut impl Print) -> Result<()> {
        Printer {
            config: self,
            result,
            code_section_hints: Vec::new(),
            group_lines: Vec::new(),
            line: 0,
            nesting: 0,
        }
        .print_contents(wasm)
    }

    /// Get the line-by-line WAT disassembly for the given Wasm, along with the
    /// binary offsets for each line.
    pub fn offsets_and_lines<'a>(
        &self,
        wasm: &[u8],
        storage: &'a mut String,
    ) -> Result<impl Iterator<Item = (Option<usize>, &'a str)> + 'a> {
        struct TrackingPrint<'a> {
            dst: &'a mut String,
            lines: Vec<usize>,
            line_offsets: Vec<Option<usize>>,
        }

        impl Print for TrackingPrint<'_> {
            fn write_str(&mut self, s: &str) -> io::Result<()> {
                self.dst.push_str(s);
                Ok(())
            }
            fn start_line(&mut self, offset: Option<usize>) {
                self.lines.push(self.dst.len());
                self.line_offsets.push(offset);
            }
        }

        let mut output = TrackingPrint {
            dst: storage,
            lines: Vec::new(),
            line_offsets: Vec::new(),
        };
        self.print(wasm, &mut output)?;

        let TrackingPrint {
            dst,
            lines,
            line_offsets,
        } = output;
        let end = dst.len();
        let dst = &dst[..];
        let mut offsets = line_offsets.into_iter();
        let mut lines = lines.into_iter().peekable();

        Ok(std::iter::from_fn(move || {
            let offset = offsets.next()?;
            let i = lines.next()?;
            let j = lines.peek().copied().unwrap_or(end);
            let line = &dst[i..j];
            Some((offset, line))
        }))
    }
}

impl Printer<'_, '_> {
    fn read_names_and_code<'a>(
        &mut self,
        mut bytes: &'a [u8],
        mut parser: Parser,
        state: &mut State,
        code: &mut Vec<FunctionBody<'a>>,
    ) -> Result<()> {
        loop {
            let payload = match parser.parse(bytes, true)? {
                Chunk::NeedMoreData(_) => unreachable!(),
                Chunk::Parsed { payload, consumed } => {
                    bytes = &bytes[consumed..];
                    payload
                }
            };

            match payload {
                Payload::FunctionSection(s) => {
                    if s.count() > MAX_WASM_FUNCTIONS {
                        bail!(
                            "module contains {} functions which exceeds the limit of {}",
                            s.count(),
                            MAX_WASM_FUNCTIONS
                        );
                    }
                    code.reserve(s.count() as usize);
                }
                Payload::CodeSectionEntry(f) => {
                    code.push(f);
                }
                Payload::ModuleSection {
                    unchecked_range: range,
                    ..
                }
                | Payload::ComponentSection {
                    unchecked_range: range,
                    ..
                } => {
                    let offset = range.end - range.start;
                    if offset > bytes.len() {
                        bail!("invalid module or component section range");
                    }
                    bytes = &bytes[offset..];
                }

                Payload::CustomSection(c) => {
                    // Ignore any error associated with the name sections.
                    match c.as_known() {
                        KnownCustom::Name(reader) => {
                            drop(self.register_names(state, reader));
                        }
                        KnownCustom::ComponentName(reader) => {
                            drop(self.register_component_names(state, reader));
                        }
                        KnownCustom::BranchHints(reader) => {
                            drop(self.register_branch_hint_section(reader));
                        }
                        _ => {}
                    }
                }

                Payload::End(_) => break,
                _ => {}
            }
        }

        Ok(())
    }

    fn ensure_module(states: &[State]) -> Result<()> {
        if !matches!(states.last().unwrap().encoding, Encoding::Module) {
            bail!("a module section was encountered when parsing a component");
        }

        Ok(())
    }

    fn ensure_component(states: &[State]) -> Result<()> {
        if !matches!(states.last().unwrap().encoding, Encoding::Component) {
            bail!("a component section was encountered when parsing a module");
        }

        Ok(())
    }

    fn print_contents(&mut self, mut bytes: &[u8]) -> Result<()> {
        self.result.start_line(Some(0));

        let mut expected = None;
        let mut states: Vec<State> = Vec::new();
        let mut parser = Parser::new(0);
        let mut parsers = Vec::new();
        let mut code = Vec::new();
        let mut code_printed = false;

        loop {
            let payload = match parser.parse(bytes, true)? {
                Chunk::NeedMoreData(_) => unreachable!(),
                Chunk::Parsed { payload, consumed } => {
                    bytes = &bytes[consumed..];
                    payload
                }
            };
            match payload {
                Payload::Version { encoding, .. } => {
                    if let Some(e) = expected {
                        if encoding != e {
                            bail!("incorrect encoding for nested module or component");
                        }
                        expected = None;
                    }

                    assert!(states.last().map(|s| s.encoding) != Some(Encoding::Module));

                    match encoding {
                        Encoding::Module => {
                            states.push(State::new(Encoding::Module));
                            states.last_mut().unwrap().custom_section_place = Some("before first");
                            if states.len() > 1 {
                                self.start_group("core module")?;
                            } else {
                                self.start_group("module")?;
                            }

                            if states.len() > 1 {
                                let parent = &states[states.len() - 2];
                                self.result.write_str(" ")?;
                                self.print_name(&parent.core.module_names, parent.core.modules)?;
                            }
                        }
                        Encoding::Component => {
                            states.push(State::new(Encoding::Component));
                            self.start_group("component")?;

                            if states.len() > 1 {
                                let parent = &states[states.len() - 2];
                                self.result.write_str(" ")?;
                                self.print_name(
                                    &parent.component.component_names,
                                    parent.component.components,
                                )?;
                            }
                        }
                    }

                    let len = states.len();
                    let state = states.last_mut().unwrap();

                    // First up try to find the `name` subsection which we'll use to print
                    // pretty names everywhere. Also look for the `code` section so we can
                    // print out functions as soon as we hit the function section.
                    code.clear();
                    code_printed = false;
                    self.read_names_and_code(bytes, parser.clone(), state, &mut code)?;

                    if len == 1 {
                        if let Some(name) = state.name.as_ref() {
                            self.result.write_str(" ")?;
                            name.write(self)?;
                        }
                    }
                }
                Payload::CustomSection(c) => {
                    // If the custom printing trait handles this section, keep
                    // going after that.
                    let printed =
                        self.result
                            .print_custom_section(c.name(), c.data_offset(), c.data())?;
                    if printed {
                        continue;
                    }

                    // If this wasn't handled specifically above then try to
                    // print the known custom builtin sections. If this fails
                    // because the custom section is malformed then don't fail
                    // the whole print.
                    let state = states.last().unwrap();
                    let start = self.nesting;
                    let err = match self.print_custom_section(state, c.clone()) {
                        Ok(()) => continue,
                        Err(e) if !e.is::<BinaryReaderError>() => return Err(e),
                        Err(e) => e,
                    };

                    // Restore the nesting level to what it was prior to
                    // starting to print the custom section. Then emit a comment
                    // indicating why the custom section failed to parse.
                    //
                    // This may leave broken syntax in the output module but
                    // it's probably better than swallowing the error.
                    while self.nesting > start {
                        self.end_group()?;
                    }
                    let msg = format!("failed to parse custom section `{}`: {err}", c.name());
                    for line in msg.lines() {
                        self.newline(c.data_offset())?;
                        write!(self.result, ";; {line}")?;
                    }
                    self.newline(c.range().end)?;
                }
                Payload::TypeSection(s) => {
                    self.update_custom_section_place(&mut states, "after type");
                    self.print_types(states.last_mut().unwrap(), s)?;
                }
                Payload::ImportSection(s) => {
                    self.update_custom_section_place(&mut states, "after import");
                    Self::ensure_module(&states)?;
                    self.print_imports(states.last_mut().unwrap(), s)?
                }
                Payload::FunctionSection(reader) => {
                    self.update_custom_section_place(&mut states, "after func");
                    Self::ensure_module(&states)?;
                    if mem::replace(&mut code_printed, true) {
                        bail!("function section appeared twice in module");
                    }
                    if reader.count() == 0 {
                        continue;
                    }
                    self.print_code(states.last_mut().unwrap(), &code, reader)?;
                }
                Payload::TableSection(s) => {
                    self.update_custom_section_place(&mut states, "after table");
                    Self::ensure_module(&states)?;
                    self.print_tables(states.last_mut().unwrap(), s)?
                }
                Payload::MemorySection(s) => {
                    self.update_custom_section_place(&mut states, "after memory");
                    Self::ensure_module(&states)?;
                    self.print_memories(states.last_mut().unwrap(), s)?
                }
                Payload::TagSection(s) => {
                    self.update_custom_section_place(&mut states, "after tag");
                    Self::ensure_module(&states)?;
                    self.print_tags(states.last_mut().unwrap(), s)?
                }
                Payload::GlobalSection(s) => {
                    self.update_custom_section_place(&mut states, "after global");
                    Self::ensure_module(&states)?;
                    self.print_globals(states.last_mut().unwrap(), s)?
                }
                Payload::ExportSection(s) => {
                    self.update_custom_section_place(&mut states, "after export");
                    Self::ensure_module(&states)?;
                    self.print_exports(states.last().unwrap(), s)?
                }
                Payload::StartSection { func, range } => {
                    self.update_custom_section_place(&mut states, "after start");
                    Self::ensure_module(&states)?;
                    self.newline(range.start)?;
                    self.start_group("start ")?;
                    self.print_idx(&states.last().unwrap().core.func_names, func)?;
                    self.end_group()?;
                }
                Payload::ElementSection(s) => {
                    self.update_custom_section_place(&mut states, "after element");
                    Self::ensure_module(&states)?;
                    self.print_elems(states.last_mut().unwrap(), s)?;
                }
                // printed with the `Function` section, so we
                // skip this section
                Payload::CodeSectionStart { size, .. } => {
                    self.update_custom_section_place(&mut states, "after code");
                    Self::ensure_module(&states)?;
                    bytes = &bytes[size as usize..];
                    parser.skip_section();
                }
                Payload::CodeSectionEntry(_) => unreachable!(),
                Payload::DataCountSection { .. } => {
                    Self::ensure_module(&states)?;
                    // not part of the text format
                }
                Payload::DataSection(s) => {
                    self.update_custom_section_place(&mut states, "after data");
                    Self::ensure_module(&states)?;
                    self.print_data(states.last_mut().unwrap(), s)?;
                }

                Payload::ModuleSection {
                    parser: inner,
                    unchecked_range: range,
                } => {
                    Self::ensure_component(&states)?;
                    expected = Some(Encoding::Module);
                    parsers.push(parser);
                    parser = inner;
                    self.newline(range.start)?;
                }
                Payload::InstanceSection(s) => {
                    Self::ensure_component(&states)?;
                    self.print_instances(states.last_mut().unwrap(), s)?;
                }
                Payload::CoreTypeSection(s) => self.print_core_types(&mut states, s)?,
                Payload::ComponentSection {
                    parser: inner,
                    unchecked_range: range,
                } => {
                    Self::ensure_component(&states)?;
                    expected = Some(Encoding::Component);
                    parsers.push(parser);
                    parser = inner;
                    self.newline(range.start)?;
                }
                Payload::ComponentInstanceSection(s) => {
                    Self::ensure_component(&states)?;
                    self.print_component_instances(states.last_mut().unwrap(), s)?;
                }
                Payload::ComponentAliasSection(s) => {
                    Self::ensure_component(&states)?;
                    self.print_component_aliases(&mut states, s)?;
                }
                Payload::ComponentTypeSection(s) => {
                    Self::ensure_component(&states)?;
                    self.print_component_types(&mut states, s)?;
                }
                Payload::ComponentCanonicalSection(s) => {
                    Self::ensure_component(&states)?;
                    self.print_canonical_functions(states.last_mut().unwrap(), s)?;
                }
                Payload::ComponentStartSection { start, range } => {
                    Self::ensure_component(&states)?;
                    self.print_component_start(states.last_mut().unwrap(), range.start, start)?;
                }
                Payload::ComponentImportSection(s) => {
                    Self::ensure_component(&states)?;
                    self.print_component_imports(states.last_mut().unwrap(), s)?;
                }
                Payload::ComponentExportSection(s) => {
                    Self::ensure_component(&states)?;
                    self.print_component_exports(states.last_mut().unwrap(), s)?;
                }

                Payload::End(offset) => {
                    self.end_group()?; // close the `module` or `component` group

                    let state = states.pop().unwrap();
                    if let Some(parent) = states.last_mut() {
                        match state.encoding {
                            Encoding::Module => {
                                parent.core.modules += 1;
                            }
                            Encoding::Component => {
                                parent.component.components += 1;
                            }
                        }
                        parser = parsers.pop().unwrap();
                    } else {
                        self.newline(offset)?;
                        if self.config.print_offsets {
                            self.result.newline()?;
                        }
                        break;
                    }
                }

                Payload::UnknownSection { id, .. } => bail!("found unknown section `{}`", id),
            }
        }

        Ok(())
    }

    fn update_custom_section_place(&self, states: &mut Vec<State>, place: &'static str) {
        if let Some(last) = states.last_mut() {
            if let Some(prev) = &mut last.custom_section_place {
                *prev = place;
            }
        }
    }

    fn start_group(&mut self, name: &str) -> Result<()> {
        write!(self.result, "(")?;
        self.result.start_keyword()?;
        write!(self.result, "{name}")?;
        self.result.reset_color()?;
        self.nesting += 1;
        self.group_lines.push(self.line);
        Ok(())
    }

    fn end_group(&mut self) -> Result<()> {
        self.nesting -= 1;
        if let Some(line) = self.group_lines.pop() {
            if line != self.line {
                self.newline_unknown_pos()?;
            }
        }
        self.result.write_str(")")?;
        Ok(())
    }

    fn register_names(&mut self, state: &mut State, names: NameSectionReader<'_>) -> Result<()> {
        fn indirect_name_map<K>(
            into: &mut NamingMap<(u32, u32), K>,
            names: IndirectNameMap<'_>,
            name: &str,
        ) -> Result<()> {
            for indirect in names {
                let indirect = indirect?;
                let mut used = match name {
                    // labels can be shadowed, so maintaining the used names is not useful.
                    "label" => None,
                    "local" | "field" => Some(HashSet::new()),
                    _ => unimplemented!("{name} is an unknown type of indirect names"),
                };
                for naming in indirect.names {
                    let naming = naming?;
                    into.index_to_name.insert(
                        (indirect.index, naming.index),
                        Naming::new(naming.name, naming.index, name, used.as_mut()),
                    );
                }
            }
            Ok(())
        }

        for section in names {
            match section? {
                Name::Module { name, .. } => {
                    let name = Naming::new(name, 0, "module", None);
                    state.name = Some(name);
                }
                Name::Function(n) => name_map(&mut state.core.func_names, n, "func")?,
                Name::Local(n) => indirect_name_map(&mut state.core.local_names, n, "local")?,
                Name::Label(n) => indirect_name_map(&mut state.core.label_names, n, "label")?,
                Name::Type(n) => name_map(&mut state.core.type_names, n, "type")?,
                Name::Table(n) => name_map(&mut state.core.table_names, n, "table")?,
                Name::Memory(n) => name_map(&mut state.core.memory_names, n, "memory")?,
                Name::Global(n) => name_map(&mut state.core.global_names, n, "global")?,
                Name::Element(n) => name_map(&mut state.core.element_names, n, "elem")?,
                Name::Data(n) => name_map(&mut state.core.data_names, n, "data")?,
                Name::Field(n) => indirect_name_map(&mut state.core.field_names, n, "field")?,
                Name::Tag(n) => name_map(&mut state.core.tag_names, n, "tag")?,
                Name::Unknown { .. } => (),
            }
        }
        Ok(())
    }

    fn register_component_names(
        &mut self,
        state: &mut State,
        names: ComponentNameSectionReader<'_>,
    ) -> Result<()> {
        for section in names {
            match section? {
                ComponentName::Component { name, .. } => {
                    let name = Naming::new(name, 0, "component", None);
                    state.name = Some(name);
                }
                ComponentName::CoreFuncs(n) => {
                    name_map(&mut state.core.func_names, n, "core-func")?
                }
                ComponentName::CoreTypes(n) => {
                    name_map(&mut state.core.type_names, n, "core-type")?
                }
                ComponentName::CoreTables(n) => {
                    name_map(&mut state.core.table_names, n, "core-table")?
                }
                ComponentName::CoreMemories(n) => {
                    name_map(&mut state.core.memory_names, n, "core-memory")?
                }
                ComponentName::CoreGlobals(n) => {
                    name_map(&mut state.core.global_names, n, "core-global")?
                }
                ComponentName::CoreModules(n) => {
                    name_map(&mut state.core.module_names, n, "core-module")?
                }
                ComponentName::CoreInstances(n) => {
                    name_map(&mut state.core.instance_names, n, "core-instance")?
                }
                ComponentName::Types(n) => name_map(&mut state.component.type_names, n, "type")?,
                ComponentName::Instances(n) => {
                    name_map(&mut state.component.instance_names, n, "instance")?
                }
                ComponentName::Components(n) => {
                    name_map(&mut state.component.component_names, n, "component")?
                }
                ComponentName::Funcs(n) => name_map(&mut state.component.func_names, n, "func")?,
                ComponentName::Values(n) => name_map(&mut state.component.value_names, n, "value")?,
                ComponentName::Unknown { .. } => (),
            }
        }
        Ok(())
    }

    fn print_core_type(&mut self, states: &mut Vec<State>, ty: CoreType) -> Result<()> {
        self.start_group("core type ")?;
        self.print_name(
            &states.last().unwrap().core.type_names,
            states.last().unwrap().core.types.len() as u32,
        )?;
        let ty = match ty {
            CoreType::Sub(ty) => {
                let ty = match &ty.composite_type.inner {
                    CompositeInnerType::Func(f) => f,
                    CompositeInnerType::Array(_) | CompositeInnerType::Struct(_) => {
                        unreachable!("Wasm GC types cannot appear in components yet")
                    }
                };
                self.result.write_str(" ")?;
                self.start_group("func")?;
                self.print_func_type(states.last().unwrap(), &ty, None)?;
                self.end_group()?;
                let composite_type = CompositeType {
                    inner: CompositeInnerType::Func(ty.clone()),
                    shared: false,
                };
                Some(SubType {
                    is_final: true,
                    supertype_idx: None,
                    composite_type,
                })
            }
            CoreType::Module(decls) => {
                self.print_module_type(states, decls.into_vec())?;
                None
            }
        };
        self.end_group()?; // `core type` itself

        states.last_mut().unwrap().core.types.push(ty);
        Ok(())
    }

    fn print_rec(
        &mut self,
        state: &mut State,
        offset: usize,
        types: impl Iterator<Item = SubType>,
    ) -> Result<()> {
        self.start_group("rec")?;
        for ty in types {
            self.newline(offset + 2)?;
            self.print_type(state, ty)?;
        }
        self.end_group()?; // `rec`
        Ok(())
    }

    fn print_type(&mut self, state: &mut State, ty: SubType) -> Result<()> {
        self.start_group("type ")?;
        let ty_idx = state.core.types.len() as u32;
        self.print_name(&state.core.type_names, ty_idx)?;
        self.result.write_str(" ")?;
        self.print_sub(state, &ty, ty_idx)?;
        self.end_group()?; // `type`
        state.core.types.push(Some(ty));
        Ok(())
    }

    fn print_sub(&mut self, state: &State, ty: &SubType, ty_idx: u32) -> Result<u32> {
        let r = if !ty.is_final || !ty.supertype_idx.is_none() {
            self.start_group("sub")?;
            self.print_sub_type(state, ty)?;
            let r = self.print_composite(state, &ty.composite_type, ty_idx)?;
            self.end_group()?; // `sub`
            r
        } else {
            self.print_composite(state, &ty.composite_type, ty_idx)?
        };
        Ok(r)
    }

    fn print_composite(&mut self, state: &State, ty: &CompositeType, ty_idx: u32) -> Result<u32> {
        if ty.shared {
            self.start_group("shared")?;
        }
        let r = match &ty.inner {
            CompositeInnerType::Func(ty) => {
                self.start_group("func")?;
                let r = self.print_func_type(state, ty, None)?;
                self.end_group()?; // `func`
                r
            }
            CompositeInnerType::Array(ty) => {
                self.start_group("array")?;
                let r = self.print_array_type(state, ty)?;
                self.end_group()?; // `array`
                r
            }
            CompositeInnerType::Struct(ty) => {
                self.start_group("struct")?;
                let r = self.print_struct_type(state, ty, ty_idx)?;
                self.end_group()?; // `struct`
                r
            }
        };
        if ty.shared {
            self.end_group()?; // `shared`
        }
        Ok(r)
    }

    fn print_core_types(
        &mut self,
        states: &mut Vec<State>,
        parser: CoreTypeSectionReader<'_>,
    ) -> Result<()> {
        for ty in parser.into_iter_with_offsets() {
            let (offset, ty) = ty?;
            self.newline(offset)?;
            self.print_core_type(states, ty)?;
        }

        Ok(())
    }

    fn print_types(&mut self, state: &mut State, parser: TypeSectionReader<'_>) -> Result<()> {
        for ty in parser.into_iter_with_offsets() {
            let (offset, rec_group) = ty?;
            self.newline(offset)?;
            if rec_group.is_explicit_rec_group() {
                self.print_rec(state, offset, rec_group.into_types())?
            } else {
                assert_eq!(rec_group.types().len(), 1);
                let ty = rec_group.into_types().next().unwrap();
                self.print_type(state, ty)?;
            }
        }

        Ok(())
    }

    fn print_core_functype_idx(
        &mut self,
        state: &State,
        idx: u32,
        names_for: Option<u32>,
    ) -> Result<Option<u32>> {
        self.print_core_type_ref(state, idx)?;

        match state.core.types.get(idx as usize) {
            Some(Some(SubType {
                composite_type:
                    CompositeType {
                        inner: CompositeInnerType::Func(ty),
                        shared: false,
                    },
                ..
            })) => self.print_func_type(state, ty, names_for).map(Some),
            Some(Some(_)) | Some(None) | None => Ok(None),
        }
    }

    /// Returns the number of parameters, useful for local index calculations
    /// later.
    fn print_func_type(
        &mut self,
        state: &State,
        ty: &FuncType,
        names_for: Option<u32>,
    ) -> Result<u32> {
        if !ty.params().is_empty() {
            self.result.write_str(" ")?;
        }

        let mut params = NamedLocalPrinter::new("param");
        // Note that named parameters must be alone in a `param` block, so
        // we need to be careful to terminate previous param blocks and open
        // a new one if that's the case with a named parameter.
        for (i, param) in ty.params().iter().enumerate() {
            params.start_local(names_for.unwrap_or(u32::MAX), i as u32, self, state)?;
            self.print_valtype(state, *param)?;
            params.end_local(self)?;
        }
        params.finish(self)?;
        if !ty.results().is_empty() {
            self.result.write_str(" ")?;
            self.start_group("result")?;
            for result in ty.results().iter() {
                self.result.write_str(" ")?;
                self.print_valtype(state, *result)?;
            }
            self.end_group()?;
        }
        Ok(ty.params().len() as u32)
    }

    fn print_field_type(
        &mut self,
        state: &State,
        ty: &FieldType,
        ty_field_idx: Option<(u32, u32)>,
    ) -> Result<u32> {
        self.result.write_str(" ")?;
        if let Some(idxs @ (_, field_idx)) = ty_field_idx {
            match state.core.field_names.index_to_name.get(&idxs) {
                Some(name) => {
                    name.write_identifier(self)?;
                    self.result.write_str(" ")?;
                }
                None if self.config.name_unnamed => write!(self.result, "$#field{field_idx} ")?,
                None => {}
            }
        }
        if ty.mutable {
            self.result.write_str("(mut ")?;
        }
        self.print_storage_type(state, ty.element_type)?;
        if ty.mutable {
            self.result.write_str(")")?;
        }
        Ok(0)
    }

    fn print_array_type(&mut self, state: &State, ty: &ArrayType) -> Result<u32> {
        self.print_field_type(state, &ty.0, None)
    }

    fn print_struct_type(&mut self, state: &State, ty: &StructType, ty_idx: u32) -> Result<u32> {
        for (field_index, field) in ty.fields.iter().enumerate() {
            self.result.write_str(" (field")?;
            self.print_field_type(state, field, Some((ty_idx, field_index as u32)))?;
            self.result.write_str(")")?;
        }
        Ok(0)
    }

    fn print_sub_type(&mut self, state: &State, ty: &SubType) -> Result<u32> {
        self.result.write_str(" ")?;
        if ty.is_final {
            self.result.write_str("final ")?;
        }
        if let Some(idx) = ty.supertype_idx {
            self.print_idx(&state.core.type_names, idx.as_module_index().unwrap())?;
            self.result.write_str(" ")?;
        }
        Ok(0)
    }

    fn print_storage_type(&mut self, state: &State, ty: StorageType) -> Result<()> {
        match ty {
            StorageType::I8 => self.result.write_str("i8")?,
            StorageType::I16 => self.result.write_str("i16")?,
            StorageType::Val(val_type) => self.print_valtype(state, val_type)?,
        }
        Ok(())
    }

    fn print_valtype(&mut self, state: &State, ty: ValType) -> Result<()> {
        match ty {
            ValType::I32 => self.print_type_keyword("i32")?,
            ValType::I64 => self.print_type_keyword("i64")?,
            ValType::F32 => self.print_type_keyword("f32")?,
            ValType::F64 => self.print_type_keyword("f64")?,
            ValType::V128 => self.print_type_keyword("v128")?,
            ValType::Ref(rt) => self.print_reftype(state, rt)?,
        }
        Ok(())
    }

    fn print_reftype(&mut self, state: &State, ty: RefType) -> Result<()> {
        if ty.is_nullable() {
            match ty.as_non_null() {
                RefType::FUNC => self.print_type_keyword("funcref")?,
                RefType::EXTERN => self.print_type_keyword("externref")?,
                RefType::I31 => self.print_type_keyword("i31ref")?,
                RefType::ANY => self.print_type_keyword("anyref")?,
                RefType::NONE => self.print_type_keyword("nullref")?,
                RefType::NOEXTERN => self.print_type_keyword("nullexternref")?,
                RefType::NOFUNC => self.print_type_keyword("nullfuncref")?,
                RefType::EQ => self.print_type_keyword("eqref")?,
                RefType::STRUCT => self.print_type_keyword("structref")?,
                RefType::ARRAY => self.print_type_keyword("arrayref")?,
                RefType::EXN => self.print_type_keyword("exnref")?,
                RefType::NOEXN => self.print_type_keyword("nullexnref")?,
                _ => {
                    self.start_group("ref")?;
                    self.result.write_str(" null ")?;
                    self.print_heaptype(state, ty.heap_type())?;
                    self.end_group()?;
                }
            }
        } else {
            self.start_group("ref ")?;
            self.print_heaptype(state, ty.heap_type())?;
            self.end_group()?;
        }
        Ok(())
    }

    fn print_heaptype(&mut self, state: &State, ty: HeapType) -> Result<()> {
        match ty {
            HeapType::Concrete(i) => {
                self.print_idx(&state.core.type_names, i.as_module_index().unwrap())?;
            }
            HeapType::Abstract { shared, ty } => {
                use AbstractHeapType::*;
                if shared {
                    self.start_group("shared ")?;
                }
                match ty {
                    Func => self.print_type_keyword("func")?,
                    Extern => self.print_type_keyword("extern")?,
                    Any => self.print_type_keyword("any")?,
                    None => self.print_type_keyword("none")?,
                    NoExtern => self.print_type_keyword("noextern")?,
                    NoFunc => self.print_type_keyword("nofunc")?,
                    Eq => self.print_type_keyword("eq")?,
                    Struct => self.print_type_keyword("struct")?,
                    Array => self.print_type_keyword("array")?,
                    I31 => self.print_type_keyword("i31")?,
                    Exn => self.print_type_keyword("exn")?,
                    NoExn => self.print_type_keyword("noexn")?,
                }
                if shared {
                    self.end_group()?;
                }
            }
        }
        Ok(())
    }

    fn print_type_keyword(&mut self, keyword: &str) -> Result<()> {
        self.result.start_type()?;
        self.result.write_str(keyword)?;
        self.result.reset_color()?;
        Ok(())
    }

    fn print_imports(&mut self, state: &mut State, parser: ImportSectionReader<'_>) -> Result<()> {
        for import in parser.into_iter_with_offsets() {
            let (offset, import) = import?;
            self.newline(offset)?;
            self.print_import(state, &import, true)?;
            match import.ty {
                TypeRef::Func(_) => state.core.funcs += 1,
                TypeRef::Table(_) => state.core.tables += 1,
                TypeRef::Memory(_) => state.core.memories += 1,
                TypeRef::Tag(_) => state.core.tags += 1,
                TypeRef::Global(_) => state.core.globals += 1,
            }
        }
        Ok(())
    }

    fn print_import(&mut self, state: &State, import: &Import<'_>, index: bool) -> Result<()> {
        self.start_group("import ")?;
        self.print_str(import.module)?;
        self.result.write_str(" ")?;
        self.print_str(import.name)?;
        self.result.write_str(" ")?;
        self.print_import_ty(state, &import.ty, index)?;
        self.end_group()?;
        Ok(())
    }

    fn print_import_ty(&mut self, state: &State, ty: &TypeRef, index: bool) -> Result<()> {
        match ty {
            TypeRef::Func(f) => {
                self.start_group("func ")?;
                if index {
                    self.print_name(&state.core.func_names, state.core.funcs)?;
                    self.result.write_str(" ")?;
                }
                self.print_core_type_ref(state, *f)?;
            }
            TypeRef::Table(f) => self.print_table_type(state, f, index)?,
            TypeRef::Memory(f) => self.print_memory_type(state, f, index)?,
            TypeRef::Tag(f) => self.print_tag_type(state, f, index)?,
            TypeRef::Global(f) => self.print_global_type(state, f, index)?,
        }
        self.end_group()?;
        Ok(())
    }

    fn print_table_type(&mut self, state: &State, ty: &TableType, index: bool) -> Result<()> {
        self.start_group("table ")?;
        if index {
            self.print_name(&state.core.table_names, state.core.tables)?;
            self.result.write_str(" ")?;
        }
        if ty.shared {
            self.print_type_keyword("shared ")?;
        }
        if ty.table64 {
            self.print_type_keyword("i64 ")?;
        }
        self.print_limits(ty.initial, ty.maximum)?;
        self.result.write_str(" ")?;
        self.print_reftype(state, ty.element_type)?;
        Ok(())
    }

    fn print_memory_type(&mut self, state: &State, ty: &MemoryType, index: bool) -> Result<()> {
        self.start_group("memory ")?;
        if index {
            self.print_name(&state.core.memory_names, state.core.memories)?;
            self.result.write_str(" ")?;
        }
        if ty.memory64 {
            self.print_type_keyword("i64 ")?;
        }
        self.print_limits(ty.initial, ty.maximum)?;
        if ty.shared {
            self.print_type_keyword(" shared")?;
        }
        if let Some(p) = ty.page_size_log2 {
            let p = 1_u64
                .checked_shl(p)
                .ok_or_else(|| anyhow!("left shift overflow").context("invalid page size"))?;
            write!(self.result, "(pagesize {p:#x})")?;
        }
        Ok(())
    }

    fn print_tag_type(&mut self, state: &State, ty: &TagType, index: bool) -> Result<()> {
        self.start_group("tag ")?;
        if index {
            self.print_name(&state.core.tag_names, state.core.tags)?;
            self.result.write_str(" ")?;
        }
        self.print_core_functype_idx(state, ty.func_type_idx, None)?;
        Ok(())
    }

    fn print_limits<T>(&mut self, initial: T, maximum: Option<T>) -> Result<()>
    where
        T: fmt::Display,
    {
        self.result.start_literal()?;
        write!(self.result, "{}", initial)?;
        if let Some(max) = maximum {
            write!(self.result, " {}", max)?;
        }
        self.result.reset_color()?;
        Ok(())
    }

    fn print_global_type(&mut self, state: &State, ty: &GlobalType, index: bool) -> Result<()> {
        self.start_group("global ")?;
        if index {
            self.print_name(&state.core.global_names, state.core.globals)?;
            self.result.write_str(" ")?;
        }
        if ty.shared || ty.mutable {
            self.result.write_str("(")?;
            if ty.shared {
                self.print_type_keyword("shared ")?;
            }
            if ty.mutable {
                self.print_type_keyword("mut ")?;
            }
            self.print_valtype(state, ty.content_type)?;
            self.result.write_str(")")?;
        } else {
            self.print_valtype(state, ty.content_type)?;
        }
        Ok(())
    }

    fn print_tables(&mut self, state: &mut State, parser: TableSectionReader<'_>) -> Result<()> {
        for table in parser.into_iter_with_offsets() {
            let (offset, table) = table?;
            self.newline(offset)?;
            self.print_table_type(state, &table.ty, true)?;
            match &table.init {
                TableInit::RefNull => {}
                TableInit::Expr(expr) => {
                    self.result.write_str(" ")?;
                    self.print_const_expr(state, expr)?;
                }
            }
            self.end_group()?;
            state.core.tables += 1;
        }
        Ok(())
    }

    fn print_memories(&mut self, state: &mut State, parser: MemorySectionReader<'_>) -> Result<()> {
        for memory in parser.into_iter_with_offsets() {
            let (offset, memory) = memory?;
            self.newline(offset)?;
            self.print_memory_type(state, &memory, true)?;
            self.end_group()?;
            state.core.memories += 1;
        }
        Ok(())
    }

    fn print_tags(&mut self, state: &mut State, parser: TagSectionReader<'_>) -> Result<()> {
        for tag in parser.into_iter_with_offsets() {
            let (offset, tag) = tag?;
            self.newline(offset)?;
            self.print_tag_type(state, &tag, true)?;
            self.end_group()?;
            state.core.tags += 1;
        }
        Ok(())
    }

    fn print_globals(&mut self, state: &mut State, parser: GlobalSectionReader<'_>) -> Result<()> {
        for global in parser.into_iter_with_offsets() {
            let (offset, global) = global?;
            self.newline(offset)?;
            self.print_global_type(state, &global.ty, true)?;
            self.result.write_str(" ")?;
            self.print_const_expr(state, &global.init_expr)?;
            self.end_group()?;
            state.core.globals += 1;
        }
        Ok(())
    }

    fn print_code(
        &mut self,
        state: &mut State,
        code: &[FunctionBody<'_>],
        funcs: FunctionSectionReader<'_>,
    ) -> Result<()> {
        if funcs.count() != code.len() as u32 {
            bail!("mismatch in function and code section counts");
        }
        for (body, ty) in code.iter().zip(funcs) {
            let mut body = body.get_binary_reader();
            let offset = body.original_position();
            let ty = ty?;
            self.newline(offset)?;
            self.start_group("func ")?;
            let func_idx = state.core.funcs;
            self.print_name(&state.core.func_names, func_idx)?;
            self.result.write_str(" ")?;
            let params = self
                .print_core_functype_idx(state, ty, Some(func_idx))?
                .unwrap_or(0);

            // Hints are stored on `self` in reverse order of function index so
            // check the last one and see if it matches this function.
            let hints = match self.code_section_hints.last() {
                Some((f, _)) if *f == func_idx => {
                    let (_, hints) = self.code_section_hints.pop().unwrap();
                    hints
                }
                _ => Vec::new(),
            };

            if self.config.print_skeleton {
                self.result.write_str(" ...")?;
            } else {
                self.print_func_body(state, func_idx, params, &mut body, &hints)?;
            }

            self.end_group()?;

            state.core.funcs += 1;
        }
        Ok(())
    }

    fn print_func_body(
        &mut self,
        state: &mut State,
        func_idx: u32,
        params: u32,
        body: &mut BinaryReader<'_>,
        mut branch_hints: &[(usize, BranchHint)],
    ) -> Result<()> {
        let mut first = true;
        let mut local_idx = 0;
        let mut locals = NamedLocalPrinter::new("local");
        let func_start = body.original_position();
        for _ in 0..body.read_var_u32()? {
            let offset = body.original_position();
            let cnt = body.read_var_u32()?;
            let ty = body.read()?;
            if MAX_LOCALS
                .checked_sub(local_idx)
                .and_then(|s| s.checked_sub(cnt))
                .is_none()
            {
                bail!("function exceeds the maximum number of locals that can be printed");
            }
            for _ in 0..cnt {
                if first {
                    self.newline(offset)?;
                    first = false;
                }
                locals.start_local(func_idx, params + local_idx, self, state)?;
                self.print_valtype(state, ty)?;
                locals.end_local(self)?;
                local_idx += 1;
            }
        }
        locals.finish(self)?;

        let nesting_start = self.nesting;

        let mut op_printer =
            operator::PrintOperator::new(self, state, operator::OperatorSeparator::Newline);
        while !body.is_end_then_eof() {
            // Branch hints are stored in increasing order of their body offset
            // so print them whenever their instruction comes up.
            if let Some(((hint_offset, hint), rest)) = branch_hints.split_first() {
                if hint.func_offset == (body.original_position() - func_start) as u32 {
                    branch_hints = rest;
                    op_printer.printer.newline(*hint_offset)?;
                    let desc = if hint.taken { "\"\\01\"" } else { "\"\\00\"" };
                    op_printer.printer.result.start_comment()?;
                    write!(
                        op_printer.printer.result,
                        "(@metadata.code.branch_hint {desc})",
                    )?;
                    op_printer.printer.result.reset_color()?;
                }
            }

            op_printer.op_offset = body.original_position();
            body.visit_operator(&mut op_printer)??;
        }

        // If this was an invalid function body then the nesting may not
        // have reset back to normal. Fix that up here and forcibly insert
        // a newline as well in case the last instruction was something
        // like an `if` which has a comment after it which could interfere
        // with the closing paren printed for the func.
        if self.nesting != nesting_start {
            self.nesting = nesting_start;
            self.newline(body.original_position())?;
        }

        Ok(())
    }

    fn newline(&mut self, offset: usize) -> Result<()> {
        self.print_newline(Some(offset))
    }

    fn newline_unknown_pos(&mut self) -> Result<()> {
        self.print_newline(None)
    }

    fn print_newline(&mut self, offset: Option<usize>) -> Result<()> {
        self.result.newline()?;
        self.result.start_line(offset);

        if self.config.print_offsets {
            match offset {
                Some(offset) => write!(self.result, "(;@{offset:<6x};)")?,
                None => self.result.write_str("           ")?,
            }
        }
        self.line += 1;

        // Clamp the maximum nesting size that we print at something somewhat
        // reasonable to avoid generating hundreds of megabytes of whitespace
        // for small-ish modules that have deep-ish nesting.
        for _ in 0..self.nesting.min(MAX_NESTING_TO_PRINT) {
            self.result.write_str("  ")?;
        }
        Ok(())
    }

    fn print_exports(&mut self, state: &State, data: ExportSectionReader) -> Result<()> {
        for export in data.into_iter_with_offsets() {
            let (offset, export) = export?;
            self.newline(offset)?;
            self.print_export(state, &export)?;
        }
        Ok(())
    }

    fn print_export(&mut self, state: &State, export: &Export) -> Result<()> {
        self.start_group("export ")?;
        self.print_str(export.name)?;
        self.result.write_str(" ")?;
        self.print_external_kind(state, export.kind, export.index)?;
        self.end_group()?; // export
        Ok(())
    }

    fn print_external_kind(&mut self, state: &State, kind: ExternalKind, index: u32) -> Result<()> {
        match kind {
            ExternalKind::Func => {
                self.start_group("func ")?;
                self.print_idx(&state.core.func_names, index)?;
            }
            ExternalKind::Table => {
                self.start_group("table ")?;
                self.print_idx(&state.core.table_names, index)?;
            }
            ExternalKind::Global => {
                self.start_group("global ")?;
                self.print_idx(&state.core.global_names, index)?;
            }
            ExternalKind::Memory => {
                self.start_group("memory ")?;
                self.print_idx(&state.core.memory_names, index)?;
            }
            ExternalKind::Tag => {
                self.start_group("tag ")?;
                write!(self.result, "{index}")?;
            }
        }
        self.end_group()?;
        Ok(())
    }

    fn print_core_type_ref(&mut self, state: &State, idx: u32) -> Result<()> {
        self.start_group("type ")?;
        self.print_idx(&state.core.type_names, idx)?;
        self.end_group()?;
        Ok(())
    }

    fn print_component_type_ref(&mut self, state: &State, idx: u32) -> Result<()> {
        self.start_group("type ")?;
        self.print_idx(&state.component.type_names, idx)?;
        self.end_group()?;
        Ok(())
    }

    fn print_idx<K>(&mut self, names: &NamingMap<u32, K>, idx: u32) -> Result<()>
    where
        K: NamingNamespace,
    {
        self._print_idx(&names.index_to_name, idx, K::desc())
    }

    fn _print_idx(&mut self, names: &HashMap<u32, Naming>, idx: u32, desc: &str) -> Result<()> {
        self.result.start_name()?;
        match names.get(&idx) {
            Some(name) => name.write_identifier(self)?,
            None if self.config.name_unnamed => write!(self.result, "$#{desc}{idx}")?,
            None => write!(self.result, "{idx}")?,
        }
        self.result.reset_color()?;
        Ok(())
    }

    fn print_local_idx(&mut self, state: &State, func: u32, idx: u32) -> Result<()> {
        self.result.start_name()?;
        match state.core.local_names.index_to_name.get(&(func, idx)) {
            Some(name) => name.write_identifier(self)?,
            None if self.config.name_unnamed => write!(self.result, "$#local{idx}")?,
            None => write!(self.result, "{}", idx)?,
        }
        self.result.reset_color()?;
        Ok(())
    }

    fn print_field_idx(&mut self, state: &State, ty: u32, idx: u32) -> Result<()> {
        self.result.start_name()?;
        match state.core.field_names.index_to_name.get(&(ty, idx)) {
            Some(name) => name.write_identifier(self)?,
            None if self.config.name_unnamed => write!(self.result, "$#field{idx}")?,
            None => write!(self.result, "{}", idx)?,
        }
        self.result.reset_color()?;
        Ok(())
    }

    fn print_name<K>(&mut self, names: &NamingMap<u32, K>, cur_idx: u32) -> Result<()>
    where
        K: NamingNamespace,
    {
        self._print_name(&names.index_to_name, cur_idx, K::desc())
    }

    fn _print_name(
        &mut self,
        names: &HashMap<u32, Naming>,
        cur_idx: u32,
        desc: &str,
    ) -> Result<()> {
        self.result.start_name()?;
        match names.get(&cur_idx) {
            Some(name) => {
                name.write(self)?;
                self.result.write_str(" ")?;
            }
            None if self.config.name_unnamed => {
                write!(self.result, "$#{desc}{cur_idx} ")?;
            }
            None => {}
        }
        write!(self.result, "(;{cur_idx};)")?;
        self.result.reset_color()?;
        Ok(())
    }

    fn print_elems(&mut self, state: &mut State, data: ElementSectionReader) -> Result<()> {
        for (i, elem) in data.into_iter_with_offsets().enumerate() {
            let (offset, mut elem) = elem?;
            self.newline(offset)?;
            self.start_group("elem ")?;
            self.print_name(&state.core.element_names, i as u32)?;
            match &mut elem.kind {
                ElementKind::Passive => {}
                ElementKind::Declared => self.result.write_str(" declare")?,
                ElementKind::Active {
                    table_index,
                    offset_expr,
                } => {
                    let table_index = table_index.unwrap_or(0);
                    if table_index != 0 {
                        self.result.write_str(" ")?;
                        self.start_group("table ")?;
                        self.print_idx(&state.core.table_names, table_index)?;
                        self.end_group()?;
                    }
                    self.result.write_str(" ")?;
                    self.print_const_expr_sugar(state, offset_expr, "offset")?;
                }
            }
            self.result.write_str(" ")?;

            if self.config.print_skeleton {
                self.result.write_str("...")?;
            } else {
                match elem.items {
                    ElementItems::Functions(reader) => {
                        self.result.write_str("func")?;
                        for idx in reader {
                            self.result.write_str(" ")?;
                            self.print_idx(&state.core.func_names, idx?)?
                        }
                    }
                    ElementItems::Expressions(ty, reader) => {
                        self.print_reftype(state, ty)?;
                        for expr in reader {
                            self.result.write_str(" ")?;
                            self.print_const_expr_sugar(state, &expr?, "item")?
                        }
                    }
                }
            }
            self.end_group()?;
        }
        Ok(())
    }

    fn print_data(&mut self, state: &mut State, data: DataSectionReader) -> Result<()> {
        for (i, data) in data.into_iter_with_offsets().enumerate() {
            let (offset, data) = data?;
            self.newline(offset)?;
            self.start_group("data ")?;
            self.print_name(&state.core.data_names, i as u32)?;
            self.result.write_str(" ")?;
            match &data.kind {
                DataKind::Passive => {}
                DataKind::Active {
                    memory_index,
                    offset_expr,
                } => {
                    if *memory_index != 0 {
                        self.start_group("memory ")?;
                        self.print_idx(&state.core.memory_names, *memory_index)?;
                        self.end_group()?;
                        self.result.write_str(" ")?;
                    }
                    self.print_const_expr_sugar(state, offset_expr, "offset")?;
                    self.result.write_str(" ")?;
                }
            }
            if self.config.print_skeleton {
                self.result.write_str("...")?;
            } else {
                self.print_bytes(data.data)?;
            }
            self.end_group()?;
        }
        Ok(())
    }

    /// Prints the operators of `expr` space-separated, taking into account that
    /// if there's only one operator in `expr` then instead of `(explicit ...)`
    /// the printing can be `(...)`.
    fn print_const_expr_sugar(
        &mut self,
        state: &mut State,
        expr: &ConstExpr,
        explicit: &str,
    ) -> Result<()> {
        self.start_group("")?;
        let mut reader = expr.get_operators_reader();

        if reader.read().is_ok() && !reader.is_end_then_eof() {
            write!(self.result, "{explicit} ")?;
        }

        self.print_const_expr(state, expr)?;

        self.end_group()?;
        Ok(())
    }

    /// Prints the operators of `expr` space-separated.
    fn print_const_expr(&mut self, state: &mut State, expr: &ConstExpr) -> Result<()> {
        let mut reader = expr.get_operators_reader();
        let mut first = true;

        let mut op_printer =
            operator::PrintOperator::new(self, state, operator::OperatorSeparator::None);
        while !reader.is_end_then_eof() {
            if first {
                first = false;
            } else {
                write!(op_printer.printer.result, " ")?;
            }
            reader.visit_operator(&mut op_printer)??;
        }
        Ok(())
    }

    fn print_primitive_val_type(&mut self, ty: &PrimitiveValType) -> Result<()> {
        match ty {
            PrimitiveValType::Bool => self.print_type_keyword("bool")?,
            PrimitiveValType::S8 => self.print_type_keyword("s8")?,
            PrimitiveValType::U8 => self.print_type_keyword("u8")?,
            PrimitiveValType::S16 => self.print_type_keyword("s16")?,
            PrimitiveValType::U16 => self.print_type_keyword("u16")?,
            PrimitiveValType::S32 => self.print_type_keyword("s32")?,
            PrimitiveValType::U32 => self.print_type_keyword("u32")?,
            PrimitiveValType::S64 => self.print_type_keyword("s64")?,
            PrimitiveValType::U64 => self.print_type_keyword("u64")?,
            PrimitiveValType::F32 => self.print_type_keyword("f32")?,
            PrimitiveValType::F64 => self.print_type_keyword("f64")?,
            PrimitiveValType::Char => self.print_type_keyword("char")?,
            PrimitiveValType::String => self.print_type_keyword("string")?,
        }
        Ok(())
    }

    fn print_record_type(
        &mut self,
        state: &State,
        fields: &[(&str, ComponentValType)],
    ) -> Result<()> {
        self.start_group("record")?;
        for (name, ty) in fields.iter() {
            self.result.write_str(" ")?;
            self.start_group("field ")?;
            self.print_str(name)?;
            self.result.write_str(" ")?;
            self.print_component_val_type(state, ty)?;
            self.end_group()?;
        }
        self.end_group()?;
        Ok(())
    }

    fn print_variant_type(&mut self, state: &State, cases: &[VariantCase]) -> Result<()> {
        self.start_group("variant")?;
        for case in cases {
            self.result.write_str(" ")?;
            self.start_group("case ")?;
            self.print_str(case.name)?;

            if let Some(ty) = case.ty {
                self.result.write_str(" ")?;
                self.print_component_val_type(state, &ty)?;
            }

            if let Some(refines) = case.refines {
                self.result.write_str(" ")?;
                self.start_group("refines ")?;
                write!(&mut self.result, "{}", refines)?;
                self.end_group()?;
            }
            self.end_group()?;
        }
        self.end_group()?;

        Ok(())
    }

    fn print_list_type(&mut self, state: &State, element_ty: &ComponentValType) -> Result<()> {
        self.start_group("list ")?;
        self.print_component_val_type(state, element_ty)?;
        self.end_group()?;
        Ok(())
    }

    fn print_tuple_type(&mut self, state: &State, tys: &[ComponentValType]) -> Result<()> {
        self.start_group("tuple")?;
        for ty in tys {
            self.result.write_str(" ")?;
            self.print_component_val_type(state, ty)?;
        }
        self.end_group()?;
        Ok(())
    }

    fn print_flag_or_enum_type(&mut self, ty: &str, names: &[&str]) -> Result<()> {
        self.start_group(ty)?;
        for name in names {
            self.result.write_str(" ")?;
            self.print_str(name)?;
        }
        self.end_group()?;
        Ok(())
    }

    fn print_option_type(&mut self, state: &State, inner: &ComponentValType) -> Result<()> {
        self.start_group("option ")?;
        self.print_component_val_type(state, inner)?;
        self.end_group()?;
        Ok(())
    }

    fn print_result_type(
        &mut self,
        state: &State,
        ok: Option<ComponentValType>,
        err: Option<ComponentValType>,
    ) -> Result<()> {
        self.start_group("result")?;

        if let Some(ok) = ok {
            self.result.write_str(" ")?;
            self.print_component_val_type(state, &ok)?;
        }

        if let Some(err) = err {
            self.result.write_str(" ")?;
            self.start_group("error ")?;
            self.print_component_val_type(state, &err)?;
            self.end_group()?;
        }

        self.end_group()?;

        Ok(())
    }

    fn print_defined_type(&mut self, state: &State, ty: &ComponentDefinedType) -> Result<()> {
        match ty {
            ComponentDefinedType::Primitive(ty) => self.print_primitive_val_type(ty)?,
            ComponentDefinedType::Record(fields) => self.print_record_type(state, fields)?,
            ComponentDefinedType::Variant(cases) => self.print_variant_type(state, cases)?,
            ComponentDefinedType::List(ty) => self.print_list_type(state, ty)?,
            ComponentDefinedType::Tuple(tys) => self.print_tuple_type(state, tys)?,
            ComponentDefinedType::Flags(names) => self.print_flag_or_enum_type("flags", names)?,
            ComponentDefinedType::Enum(cases) => self.print_flag_or_enum_type("enum", cases)?,
            ComponentDefinedType::Option(ty) => self.print_option_type(state, ty)?,
            ComponentDefinedType::Result { ok, err } => self.print_result_type(state, *ok, *err)?,
            ComponentDefinedType::Own(idx) => {
                self.start_group("own ")?;
                self.print_idx(&state.component.type_names, *idx)?;
                self.end_group()?;
            }
            ComponentDefinedType::Borrow(idx) => {
                self.start_group("borrow ")?;
                self.print_idx(&state.component.type_names, *idx)?;
                self.end_group()?;
            }
        }

        Ok(())
    }

    fn print_component_val_type(&mut self, state: &State, ty: &ComponentValType) -> Result<()> {
        match ty {
            ComponentValType::Primitive(ty) => self.print_primitive_val_type(ty),
            ComponentValType::Type(idx) => self.print_idx(&state.component.type_names, *idx),
        }
    }

    fn print_module_type(
        &mut self,
        states: &mut Vec<State>,
        decls: Vec<ModuleTypeDeclaration>,
    ) -> Result<()> {
        states.push(State::new(Encoding::Module));
        self.newline_unknown_pos()?;
        self.start_group("module")?;
        for decl in decls {
            self.newline_unknown_pos()?;
            match decl {
                ModuleTypeDeclaration::Type(ty) => {
                    self.print_type(states.last_mut().unwrap(), ty)?
                }
                ModuleTypeDeclaration::OuterAlias { kind, count, index } => {
                    self.print_outer_alias(states, kind, count, index)?;
                }
                ModuleTypeDeclaration::Export { name, ty } => {
                    self.start_group("export ")?;
                    self.print_str(name)?;
                    self.result.write_str(" ")?;
                    self.print_import_ty(states.last_mut().unwrap(), &ty, false)?;
                    self.end_group()?;
                }
                ModuleTypeDeclaration::Import(import) => {
                    self.print_import(states.last_mut().unwrap(), &import, false)?;
                }
            }
        }
        self.end_group()?;
        states.pop();
        Ok(())
    }

    fn print_component_type<'a>(
        &mut self,
        states: &mut Vec<State>,
        decls: Vec<ComponentTypeDeclaration<'a>>,
    ) -> Result<()> {
        states.push(State::new(Encoding::Component));
        self.newline_unknown_pos()?;
        self.start_group("component")?;
        for decl in decls {
            self.newline_unknown_pos()?;
            match decl {
                ComponentTypeDeclaration::CoreType(ty) => self.print_core_type(states, ty)?,
                ComponentTypeDeclaration::Type(ty) => self.print_component_type_def(states, ty)?,
                ComponentTypeDeclaration::Alias(alias) => {
                    self.print_component_alias(states, alias)?;
                }
                ComponentTypeDeclaration::Export { name, ty } => {
                    self.start_group("export ")?;
                    self.print_component_kind_name(states.last_mut().unwrap(), ty.kind())?;
                    self.result.write_str(" ")?;
                    self.print_str(name.0)?;
                    self.result.write_str(" ")?;
                    self.print_component_import_ty(states.last_mut().unwrap(), &ty, false)?;
                    self.end_group()?;
                }
                ComponentTypeDeclaration::Import(import) => {
                    self.print_component_import(states.last_mut().unwrap(), &import, true)?
                }
            }
        }
        self.end_group()?;
        states.pop().unwrap();
        Ok(())
    }

    fn print_instance_type<'a>(
        &mut self,
        states: &mut Vec<State>,
        decls: Vec<InstanceTypeDeclaration<'a>>,
    ) -> Result<()> {
        states.push(State::new(Encoding::Component));
        self.newline_unknown_pos()?;
        self.start_group("instance")?;
        for decl in decls {
            self.newline_unknown_pos()?;
            match decl {
                InstanceTypeDeclaration::CoreType(ty) => self.print_core_type(states, ty)?,
                InstanceTypeDeclaration::Type(ty) => self.print_component_type_def(states, ty)?,
                InstanceTypeDeclaration::Alias(alias) => {
                    self.print_component_alias(states, alias)?;
                }
                InstanceTypeDeclaration::Export { name, ty } => {
                    self.start_group("export ")?;
                    self.print_component_kind_name(states.last_mut().unwrap(), ty.kind())?;
                    self.result.write_str(" ")?;
                    self.print_str(name.0)?;
                    self.result.write_str(" ")?;
                    self.print_component_import_ty(states.last_mut().unwrap(), &ty, false)?;
                    self.end_group()?;
                }
            }
        }
        self.end_group()?;
        states.pop().unwrap();
        Ok(())
    }

    fn outer_state(states: &[State], count: u32) -> Result<&State> {
        let count = count as usize;
        if count >= states.len() {
            bail!("invalid outer alias count of {}", count);
        }

        Ok(&states[states.len() - count - 1])
    }

    fn print_outer_alias(
        &mut self,
        states: &mut [State],
        kind: OuterAliasKind,
        count: u32,
        index: u32,
    ) -> Result<()> {
        let state = states.last().unwrap();
        let outer = Self::outer_state(states, count)?;
        self.start_group("alias outer ")?;
        if let Some(name) = outer.name.as_ref() {
            name.write(self)?;
        } else {
            write!(self.result, "{count}")?;
        }
        self.result.write_str(" ")?;
        match kind {
            OuterAliasKind::Type => {
                self.print_idx(&outer.core.type_names, index)?;
                self.result.write_str(" ")?;
                self.start_group("type ")?;
                self.print_name(&state.core.type_names, state.core.types.len() as u32)?;
            }
        }
        self.end_group()?; // kind
        self.end_group()?; // alias

        let state = states.last_mut().unwrap();
        match kind {
            OuterAliasKind::Type => state.core.types.push(None),
        }

        Ok(())
    }

    fn print_component_func_type(&mut self, state: &State, ty: &ComponentFuncType) -> Result<()> {
        self.start_group("func")?;
        for (name, ty) in ty.params.iter() {
            self.result.write_str(" ")?;
            self.start_group("param ")?;
            self.print_str(name)?;
            self.result.write_str(" ")?;
            self.print_component_val_type(state, ty)?;
            self.end_group()?;
        }

        for (name, ty) in ty.results.iter() {
            self.result.write_str(" ")?;
            self.start_group("result ")?;
            if let Some(name) = name {
                self.print_str(name)?;
                self.result.write_str(" ")?;
            }
            self.print_component_val_type(state, ty)?;
            self.end_group()?;
        }

        self.end_group()?;

        Ok(())
    }

    fn print_component_type_def<'a>(
        &mut self,
        states: &mut Vec<State>,
        ty: ComponentType<'a>,
    ) -> Result<()> {
        self.start_group("type ")?;
        {
            let state = states.last_mut().unwrap();
            self.print_name(&state.component.type_names, state.component.types)?;
        }
        match ty {
            ComponentType::Defined(ty) => {
                self.result.write_str(" ")?;
                self.print_defined_type(states.last_mut().unwrap(), &ty)?;
            }
            ComponentType::Func(ty) => {
                self.result.write_str(" ")?;
                self.print_component_func_type(states.last_mut().unwrap(), &ty)?;
            }
            ComponentType::Component(decls) => {
                self.print_component_type(states, decls.into_vec())?;
            }
            ComponentType::Instance(decls) => {
                self.print_instance_type(states, decls.into_vec())?;
            }
            ComponentType::Resource { rep, dtor } => {
                self.result.write_str(" ")?;
                self.start_group("resource ")?;
                self.start_group("rep ")?;
                self.print_valtype(states.last().unwrap(), rep)?;
                self.end_group()?;
                if let Some(dtor) = dtor {
                    self.result.write_str(" ")?;
                    self.start_group("dtor ")?;
                    self.start_group("func ")?;
                    self.print_idx(&states.last().unwrap().core.func_names, dtor)?;
                    self.end_group()?;
                    self.end_group()?;
                }
                self.end_group()?;
            }
        }
        self.end_group()?;

        states.last_mut().unwrap().component.types += 1;

        Ok(())
    }

    fn print_component_types<'a>(
        &mut self,
        states: &mut Vec<State>,
        parser: ComponentTypeSectionReader<'a>,
    ) -> Result<()> {
        for ty in parser.into_iter_with_offsets() {
            let (offset, ty) = ty?;
            self.newline(offset)?;
            self.print_component_type_def(states, ty)?;
        }

        Ok(())
    }

    fn print_component_imports(
        &mut self,
        state: &mut State,
        parser: ComponentImportSectionReader,
    ) -> Result<()> {
        for import in parser.into_iter_with_offsets() {
            let (offset, import) = import?;
            self.newline(offset)?;
            self.print_component_import(state, &import, true)?;
        }

        Ok(())
    }

    fn print_component_import(
        &mut self,
        state: &mut State,
        import: &ComponentImport,
        index: bool,
    ) -> Result<()> {
        self.start_group("import ")?;
        self.print_str(import.name.0)?;
        self.result.write_str(" ")?;
        self.print_component_import_ty(state, &import.ty, index)?;
        self.end_group()?;
        Ok(())
    }

    fn print_component_import_ty(
        &mut self,
        state: &mut State,
        ty: &ComponentTypeRef,
        index: bool,
    ) -> Result<()> {
        match ty {
            ComponentTypeRef::Module(idx) => {
                self.start_group("core module ")?;
                if index {
                    self.print_name(&state.core.module_names, state.core.modules as u32)?;
                    self.result.write_str(" ")?;
                    state.core.modules += 1;
                }
                self.print_component_type_ref(state, *idx)?;
                self.end_group()?;
            }
            ComponentTypeRef::Func(idx) => {
                self.start_group("func ")?;
                if index {
                    self.print_name(&state.component.func_names, state.component.funcs)?;
                    self.result.write_str(" ")?;
                    state.component.funcs += 1;
                }
                self.print_component_type_ref(state, *idx)?;
                self.end_group()?;
            }
            ComponentTypeRef::Value(ty) => {
                self.start_group("value ")?;
                if index {
                    self.print_name(&state.component.value_names, state.component.values)?;
                    self.result.write_str(" ")?;
                    state.component.values += 1;
                }
                match ty {
                    ComponentValType::Primitive(ty) => self.print_primitive_val_type(ty)?,
                    ComponentValType::Type(idx) => self.print_component_type_ref(state, *idx)?,
                }
                self.end_group()?;
            }
            ComponentTypeRef::Type(bounds) => {
                self.start_group("type ")?;
                if index {
                    self.print_name(&state.component.type_names, state.component.types)?;
                    self.result.write_str(" ")?;
                    state.component.types += 1;
                }
                match bounds {
                    TypeBounds::Eq(idx) => {
                        self.start_group("eq ")?;
                        self.print_idx(&state.component.type_names, *idx)?;
                        self.end_group()?;
                    }
                    TypeBounds::SubResource => {
                        self.start_group("sub ")?;
                        self.print_type_keyword("resource")?;
                        self.end_group()?;
                    }
                };
                self.end_group()?;
            }
            ComponentTypeRef::Instance(idx) => {
                self.start_group("instance ")?;
                if index {
                    self.print_name(&state.component.instance_names, state.component.instances)?;
                    self.result.write_str(" ")?;
                    state.component.instances += 1;
                }
                self.print_component_type_ref(state, *idx)?;
                self.end_group()?;
            }
            ComponentTypeRef::Component(idx) => {
                self.start_group("component ")?;
                if index {
                    self.print_name(&state.component.component_names, state.component.components)?;
                    self.result.write_str(" ")?;
                    state.component.components += 1;
                }
                self.print_component_type_ref(state, *idx)?;
                self.end_group()?;
            }
        }
        Ok(())
    }

    fn print_component_exports(
        &mut self,
        state: &mut State,
        parser: ComponentExportSectionReader,
    ) -> Result<()> {
        for export in parser.into_iter_with_offsets() {
            let (offset, export) = export?;
            self.newline(offset)?;
            self.print_component_export(state, &export, true)?;
        }
        Ok(())
    }

    fn print_component_export(
        &mut self,
        state: &mut State,
        export: &ComponentExport,
        named: bool,
    ) -> Result<()> {
        self.start_group("export ")?;
        if named {
            self.print_component_kind_name(state, export.kind)?;
            self.result.write_str(" ")?;
        }
        self.print_str(export.name.0)?;
        self.result.write_str(" ")?;
        self.print_component_external_kind(state, export.kind, export.index)?;
        if let Some(ty) = &export.ty {
            self.result.write_str(" ")?;
            self.print_component_import_ty(state, &ty, false)?;
        }
        self.end_group()?;
        Ok(())
    }

    fn print_component_kind_name(
        &mut self,
        state: &mut State,
        kind: ComponentExternalKind,
    ) -> Result<()> {
        match kind {
            ComponentExternalKind::Func => {
                self.print_name(&state.component.func_names, state.component.funcs)?;
                state.component.funcs += 1;
            }
            ComponentExternalKind::Module => {
                self.print_name(&state.core.module_names, state.core.modules)?;
                state.core.modules += 1;
            }
            ComponentExternalKind::Value => {
                self.print_name(&state.component.value_names, state.component.values)?;
                state.component.values += 1;
            }
            ComponentExternalKind::Type => {
                self.print_name(&state.component.type_names, state.component.types)?;
                state.component.types += 1;
            }
            ComponentExternalKind::Instance => {
                self.print_name(&state.component.instance_names, state.component.instances)?;
                state.component.instances += 1;
            }
            ComponentExternalKind::Component => {
                self.print_name(&state.component.component_names, state.component.components)?;
                state.component.components += 1;
            }
        }
        Ok(())
    }

    fn start_component_external_kind_group(&mut self, kind: ComponentExternalKind) -> Result<()> {
        match kind {
            ComponentExternalKind::Module => {
                self.start_group("core module ")?;
            }
            ComponentExternalKind::Component => {
                self.start_group("component ")?;
            }
            ComponentExternalKind::Instance => {
                self.start_group("instance ")?;
            }
            ComponentExternalKind::Func => {
                self.start_group("func ")?;
            }
            ComponentExternalKind::Value => {
                self.start_group("value ")?;
            }
            ComponentExternalKind::Type => {
                self.start_group("type ")?;
            }
        }
        Ok(())
    }

    fn print_component_external_kind(
        &mut self,
        state: &State,
        kind: ComponentExternalKind,
        index: u32,
    ) -> Result<()> {
        self.start_component_external_kind_group(kind)?;
        match kind {
            ComponentExternalKind::Module => {
                self.print_idx(&state.core.module_names, index)?;
            }
            ComponentExternalKind::Component => {
                self.print_idx(&state.component.component_names, index)?;
            }
            ComponentExternalKind::Instance => {
                self.print_idx(&state.component.instance_names, index)?;
            }
            ComponentExternalKind::Func => {
                self.print_idx(&state.component.func_names, index)?;
            }
            ComponentExternalKind::Value => {
                self.print_idx(&state.component.value_names, index)?;
            }
            ComponentExternalKind::Type => {
                self.print_idx(&state.component.type_names, index)?;
            }
        }
        self.end_group()?;
        Ok(())
    }

    fn print_canonical_options(
        &mut self,
        state: &State,
        options: &[CanonicalOption],
    ) -> Result<()> {
        for option in options {
            self.result.write_str(" ")?;
            match option {
                CanonicalOption::UTF8 => self.result.write_str("string-encoding=utf8")?,
                CanonicalOption::UTF16 => self.result.write_str("string-encoding=utf16")?,
                CanonicalOption::CompactUTF16 => {
                    self.result.write_str("string-encoding=latin1+utf16")?
                }
                CanonicalOption::Memory(idx) => {
                    self.start_group("memory ")?;
                    self.print_idx(&state.core.memory_names, *idx)?;
                    self.end_group()?;
                }
                CanonicalOption::Realloc(idx) => {
                    self.start_group("realloc ")?;
                    self.print_idx(&state.core.func_names, *idx)?;
                    self.end_group()?;
                }
                CanonicalOption::PostReturn(idx) => {
                    self.start_group("post-return ")?;
                    self.print_idx(&state.core.func_names, *idx)?;
                    self.end_group()?;
                }
            }
        }
        Ok(())
    }

    fn print_canonical_functions(
        &mut self,
        state: &mut State,
        parser: ComponentCanonicalSectionReader,
    ) -> Result<()> {
        for func in parser.into_iter_with_offsets() {
            let (offset, func) = func?;
            self.newline(offset)?;
            match func {
                CanonicalFunction::Lift {
                    core_func_index,
                    type_index,
                    options,
                } => {
                    self.start_group("func ")?;
                    self.print_name(&state.component.func_names, state.component.funcs)?;
                    self.result.write_str(" ")?;
                    self.start_group("type ")?;
                    self.print_idx(&state.component.type_names, type_index)?;
                    self.end_group()?;
                    self.result.write_str(" ")?;
                    self.start_group("canon lift ")?;
                    self.start_group("core func ")?;
                    self.print_idx(&state.core.func_names, core_func_index)?;
                    self.end_group()?;
                    self.print_canonical_options(state, &options)?;
                    self.end_group()?;
                    self.end_group()?;
                    state.component.funcs += 1;
                }
                CanonicalFunction::Lower {
                    func_index,
                    options,
                } => {
                    self.start_group("core func ")?;
                    self.print_name(&state.core.func_names, state.core.funcs)?;
                    self.result.write_str(" ")?;
                    self.start_group("canon lower ")?;
                    self.start_group("func ")?;
                    self.print_idx(&state.component.func_names, func_index)?;
                    self.end_group()?;
                    self.print_canonical_options(state, &options)?;
                    self.end_group()?;
                    self.end_group()?;
                    state.core.funcs += 1;
                }
                CanonicalFunction::ResourceNew { resource } => {
                    self.start_group("core func ")?;
                    self.print_name(&state.core.func_names, state.core.funcs)?;
                    self.result.write_str(" ")?;
                    self.start_group("canon resource.new ")?;
                    self.print_idx(&state.component.type_names, resource)?;
                    self.end_group()?;
                    self.end_group()?;
                    state.core.funcs += 1;
                }
                CanonicalFunction::ResourceDrop { resource } => {
                    self.start_group("core func ")?;
                    self.print_name(&state.core.func_names, state.core.funcs)?;
                    self.result.write_str(" ")?;
                    self.start_group("canon resource.drop ")?;
                    self.print_idx(&state.component.type_names, resource)?;
                    self.end_group()?;
                    self.end_group()?;
                    state.core.funcs += 1;
                }
                CanonicalFunction::ResourceRep { resource } => {
                    self.start_group("core func ")?;
                    self.print_name(&state.core.func_names, state.core.funcs)?;
                    self.result.write_str(" ")?;
                    self.start_group("canon resource.rep ")?;
                    self.print_idx(&state.component.type_names, resource)?;
                    self.end_group()?;
                    self.end_group()?;
                    state.core.funcs += 1;
                }
            }
        }

        Ok(())
    }

    fn print_instances(&mut self, state: &mut State, parser: InstanceSectionReader) -> Result<()> {
        for instance in parser.into_iter_with_offsets() {
            let (offset, instance) = instance?;
            self.newline(offset)?;
            self.start_group("core instance ")?;
            self.print_name(&state.core.instance_names, state.core.instances)?;
            match instance {
                Instance::Instantiate { module_index, args } => {
                    self.result.write_str(" ")?;
                    self.start_group("instantiate ")?;
                    self.print_idx(&state.core.module_names, module_index)?;
                    for arg in args.iter() {
                        self.newline(offset)?;
                        self.print_instantiation_arg(state, arg)?;
                    }
                    self.end_group()?;
                    state.core.instances += 1;
                }
                Instance::FromExports(exports) => {
                    for export in exports.iter() {
                        self.newline(offset)?;
                        self.print_export(state, export)?;
                    }
                    state.core.instances += 1;
                }
            }
            self.end_group()?;
        }
        Ok(())
    }

    fn print_component_instances(
        &mut self,
        state: &mut State,
        parser: ComponentInstanceSectionReader,
    ) -> Result<()> {
        for instance in parser.into_iter_with_offsets() {
            let (offset, instance) = instance?;
            self.newline(offset)?;
            self.start_group("instance ")?;
            self.print_name(&state.component.instance_names, state.component.instances)?;
            state.component.instances += 1;
            match instance {
                ComponentInstance::Instantiate {
                    component_index,
                    args,
                } => {
                    self.result.write_str(" ")?;
                    self.start_group("instantiate ")?;
                    self.print_idx(&state.component.component_names, component_index)?;
                    for arg in args.iter() {
                        self.newline(offset)?;
                        self.print_component_instantiation_arg(state, arg)?;
                    }
                    self.end_group()?;
                }
                ComponentInstance::FromExports(exports) => {
                    for export in exports.iter() {
                        self.newline(offset)?;
                        self.print_component_export(state, export, false)?;
                    }
                }
            }
            self.end_group()?;
        }
        Ok(())
    }

    fn print_instantiation_arg(&mut self, state: &State, arg: &InstantiationArg) -> Result<()> {
        self.start_group("with ")?;
        self.print_str(arg.name)?;
        self.result.write_str(" ")?;
        match arg.kind {
            InstantiationArgKind::Instance => {
                self.start_group("instance ")?;
                self.print_idx(&state.core.instance_names, arg.index)?;
                self.end_group()?;
            }
        }
        self.end_group()?;
        Ok(())
    }

    fn print_component_instantiation_arg(
        &mut self,
        state: &State,
        arg: &ComponentInstantiationArg,
    ) -> Result<()> {
        self.start_group("with ")?;
        self.print_str(arg.name)?;
        self.result.write_str(" ")?;
        self.print_component_external_kind(state, arg.kind, arg.index)?;
        self.end_group()?;
        Ok(())
    }

    fn print_component_start(
        &mut self,
        state: &mut State,
        pos: usize,
        start: ComponentStartFunction,
    ) -> Result<()> {
        self.newline(pos)?;
        self.start_group("start ")?;
        self.print_idx(&state.component.func_names, start.func_index)?;

        for arg in start.arguments.iter() {
            self.result.write_str(" ")?;
            self.start_group("value ")?;
            self.print_idx(&state.component.value_names, *arg)?;
            self.end_group()?;
        }

        for _ in 0..start.results {
            self.result.write_str(" ")?;
            self.start_group("result ")?;
            self.start_group("value ")?;
            self.print_name(&state.component.value_names, state.component.values)?;
            self.end_group()?;
            self.end_group()?;
            state.component.values += 1;
        }

        self.end_group()?; // start

        Ok(())
    }

    fn print_component_aliases(
        &mut self,
        states: &mut [State],
        parser: ComponentAliasSectionReader,
    ) -> Result<()> {
        for alias in parser.into_iter_with_offsets() {
            let (offset, alias) = alias?;
            self.newline(offset)?;
            self.print_component_alias(states, alias)?;
        }
        Ok(())
    }

    fn print_component_alias(
        &mut self,
        states: &mut [State],
        alias: ComponentAlias<'_>,
    ) -> Result<()> {
        match alias {
            ComponentAlias::InstanceExport {
                kind,
                instance_index,
                name,
            } => {
                let state = states.last_mut().unwrap();
                self.start_group("alias export ")?;
                self.print_idx(&state.component.instance_names, instance_index)?;
                self.result.write_str(" ")?;
                self.print_str(name)?;
                self.result.write_str(" ")?;
                self.start_component_external_kind_group(kind)?;
                self.print_component_kind_name(state, kind)?;
                self.end_group()?;

                self.end_group()?; // alias export
            }
            ComponentAlias::CoreInstanceExport {
                instance_index,
                kind,
                name,
            } => {
                let state = states.last_mut().unwrap();
                self.start_group("alias core export ")?;
                self.print_idx(&state.core.instance_names, instance_index)?;
                self.result.write_str(" ")?;
                self.print_str(name)?;
                self.result.write_str(" ")?;
                match kind {
                    ExternalKind::Func => {
                        self.start_group("core func ")?;
                        self.print_name(&state.core.func_names, state.core.funcs)?;
                        self.end_group()?;
                        state.core.funcs += 1;
                    }
                    ExternalKind::Table => {
                        self.start_group("core table ")?;
                        self.print_name(&state.core.table_names, state.core.tables)?;
                        self.end_group()?;
                        state.core.tables += 1;
                    }
                    ExternalKind::Memory => {
                        self.start_group("core memory ")?;
                        self.print_name(&state.core.memory_names, state.core.memories)?;
                        self.end_group()?;
                        state.core.memories += 1;
                    }
                    ExternalKind::Global => {
                        self.start_group("core global ")?;
                        self.print_name(&state.core.global_names, state.core.globals)?;
                        self.end_group()?;
                        state.core.globals += 1;
                    }
                    ExternalKind::Tag => {
                        self.start_group("core tag ")?;
                        self.print_name(&state.core.tag_names, state.core.tags)?;
                        self.end_group()?;
                        state.core.tags += 1;
                    }
                }
                self.end_group()?; // alias export
            }

            ComponentAlias::Outer { kind, count, index } => {
                let state = states.last().unwrap();
                let outer = Self::outer_state(states, count)?;
                self.start_group("alias outer ")?;
                if let Some(name) = outer.name.as_ref() {
                    name.write(self)?;
                } else {
                    write!(self.result, "{count}")?;
                }
                self.result.write_str(" ")?;
                match kind {
                    ComponentOuterAliasKind::CoreModule => {
                        self.print_idx(&outer.core.module_names, index)?;
                        self.result.write_str(" ")?;
                        self.start_group("core module ")?;
                        self.print_name(&state.core.module_names, state.core.modules)?;
                    }
                    ComponentOuterAliasKind::CoreType => {
                        self.print_idx(&outer.core.type_names, index)?;
                        self.result.write_str(" ")?;
                        self.start_group("core type ")?;
                        self.print_name(&state.core.type_names, state.core.types.len() as u32)?;
                    }
                    ComponentOuterAliasKind::Type => {
                        self.print_idx(&outer.component.type_names, index)?;
                        self.result.write_str(" ")?;
                        self.start_group("type ")?;
                        self.print_name(&state.component.type_names, state.component.types)?;
                    }
                    ComponentOuterAliasKind::Component => {
                        self.print_idx(&outer.component.component_names, index)?;
                        self.result.write_str(" ")?;
                        self.start_group("component ")?;
                        self.print_name(
                            &state.component.component_names,
                            state.component.components,
                        )?;
                    }
                }
                self.end_group()?; // kind
                self.end_group()?; // alias

                let state = states.last_mut().unwrap();
                match kind {
                    ComponentOuterAliasKind::CoreModule => state.core.modules += 1,
                    ComponentOuterAliasKind::CoreType => state.core.types.push(None),
                    ComponentOuterAliasKind::Type => state.component.types += 1,
                    ComponentOuterAliasKind::Component => state.component.components += 1,
                }
            }
        }
        Ok(())
    }

    fn print_str(&mut self, name: &str) -> Result<()> {
        self.result.start_literal()?;
        self.result.write_str("\"")?;
        self.print_str_contents(name)?;
        self.result.write_str("\"")?;
        self.result.reset_color()?;
        Ok(())
    }

    fn print_str_contents(&mut self, name: &str) -> Result<()> {
        for c in name.chars() {
            let v = c as u32;
            if (0x20..0x7f).contains(&v) && c != '"' && c != '\\' && v < 0xff {
                write!(self.result, "{c}")?;
            } else {
                write!(self.result, "\\u{{{v:x}}}",)?;
            }
        }
        Ok(())
    }

    fn print_bytes(&mut self, bytes: &[u8]) -> Result<()> {
        self.result.start_literal()?;
        self.result.write_str("\"")?;
        for byte in bytes {
            if *byte >= 0x20 && *byte < 0x7f && *byte != b'"' && *byte != b'\\' {
                write!(self.result, "{}", *byte as char)?;
            } else {
                self.hex_byte(*byte)?;
            }
        }
        self.result.write_str("\"")?;
        self.result.reset_color()?;
        Ok(())
    }

    fn hex_byte(&mut self, byte: u8) -> Result<()> {
        write!(self.result, "\\{byte:02x}")?;
        Ok(())
    }

    fn print_custom_section(
        &mut self,
        state: &State,
        section: CustomSectionReader<'_>,
    ) -> Result<()> {
        match section.as_known() {
            // For now `wasmprinter` has invented syntax for `producers` and
            // `dylink.0` below to use in tests. Note that this syntax is not
            // official at this time.
            KnownCustom::Producers(s) => {
                self.newline(section.range().start)?;
                self.print_producers_section(s)
            }
            KnownCustom::Dylink0(s) => {
                self.newline(section.range().start)?;
                self.print_dylink0_section(s)
            }

            // These are parsed during `read_names_and_code` and are part of
            // printing elsewhere, so don't print them.
            KnownCustom::Name(_) | KnownCustom::ComponentName(_) | KnownCustom::BranchHints(_) => {
                Ok(())
            }

            // Custom sections without a text format at this time and unknown
            // custom sections get a `@custom` annotation printed.
            KnownCustom::CoreDump(_)
            | KnownCustom::CoreDumpModules(_)
            | KnownCustom::CoreDumpStack(_)
            | KnownCustom::CoreDumpInstances(_)
            | KnownCustom::Linking(_)
            | KnownCustom::Reloc(_)
            | KnownCustom::Unknown => self.print_raw_custom_section(state, section),
        }
    }

    fn print_raw_custom_section(
        &mut self,
        state: &State,
        section: CustomSectionReader<'_>,
    ) -> Result<()> {
        self.newline(section.range().start)?;
        self.start_group("@custom ")?;
        self.print_str(section.name())?;
        if let Some(place) = state.custom_section_place {
            write!(self.result, " ({place})")?;
        }
        self.result.write_str(" ")?;
        self.print_bytes(section.data())?;
        self.end_group()?;
        Ok(())
    }

    fn print_producers_section(&mut self, section: ProducersSectionReader<'_>) -> Result<()> {
        self.start_group("@producers")?;
        for field in section {
            let field = field?;
            for value in field.values.into_iter_with_offsets() {
                let (offset, value) = value?;
                self.newline(offset)?;
                self.start_group(field.name)?;
                self.result.write_str(" ")?;
                self.print_str(value.name)?;
                self.result.write_str(" ")?;
                self.print_str(value.version)?;
                self.end_group()?;
            }
        }
        self.end_group()?;
        Ok(())
    }

    fn print_dylink0_section(&mut self, mut section: Dylink0SectionReader<'_>) -> Result<()> {
        self.start_group("@dylink.0")?;
        loop {
            let start = section.original_position();
            let next = match section.next() {
                Some(Ok(next)) => next,
                Some(Err(e)) => return Err(e.into()),
                None => break,
            };
            match next {
                Dylink0Subsection::MemInfo(info) => {
                    self.newline(start)?;
                    self.start_group("mem-info")?;
                    if info.memory_size > 0 || info.memory_alignment > 0 {
                        write!(
                            self.result,
                            " (memory {} {})",
                            info.memory_size, info.memory_alignment
                        )?;
                    }
                    if info.table_size > 0 || info.table_alignment > 0 {
                        write!(
                            self.result,
                            " (table {} {})",
                            info.table_size, info.table_alignment
                        )?;
                    }
                    self.end_group()?;
                }
                Dylink0Subsection::Needed(needed) => {
                    self.newline(start)?;
                    self.start_group("needed")?;
                    for s in needed {
                        self.result.write_str(" ")?;
                        self.print_str(s)?;
                    }
                    self.end_group()?;
                }
                Dylink0Subsection::ExportInfo(info) => {
                    for info in info {
                        self.newline(start)?;
                        self.start_group("export-info ")?;
                        self.print_str(info.name)?;
                        self.print_dylink0_flags(info.flags)?;
                        self.end_group()?;
                    }
                }
                Dylink0Subsection::ImportInfo(info) => {
                    for info in info {
                        self.newline(start)?;
                        self.start_group("import-info ")?;
                        self.print_str(info.module)?;
                        self.result.write_str(" ")?;
                        self.print_str(info.field)?;
                        self.print_dylink0_flags(info.flags)?;
                        self.end_group()?;
                    }
                }
                Dylink0Subsection::Unknown { ty, .. } => {
                    bail!("don't know how to print dylink.0 subsection id {ty}");
                }
            }
        }
        self.end_group()?;
        Ok(())
    }

    fn print_dylink0_flags(&mut self, mut flags: SymbolFlags) -> Result<()> {
        macro_rules! print_flag {
            ($($name:ident = $text:tt)*) => ({$(
                if flags.contains(SymbolFlags::$name) {
                    flags.remove(SymbolFlags::$name);
                    write!(self.result, concat!(" ", $text))?;
                }
            )*})
        }
        // N.B.: Keep in sync with `parse_sym_flags` in `crates/wast/src/core/custom.rs`.
        print_flag! {
            BINDING_WEAK = "binding-weak"
            BINDING_LOCAL = "binding-local"
            VISIBILITY_HIDDEN = "visibility-hidden"
            UNDEFINED = "undefined"
            EXPORTED = "exported"
            EXPLICIT_NAME = "explicit-name"
            NO_STRIP = "no-strip"
            TLS = "tls"
            ABSOLUTE = "absolute"
        }
        if !flags.is_empty() {
            write!(self.result, " {:#x}", flags)?;
        }
        Ok(())
    }

    fn register_branch_hint_section(&mut self, section: BranchHintSectionReader<'_>) -> Result<()> {
        self.code_section_hints.clear();
        for func in section {
            let func = func?;
            if self.code_section_hints.len() >= MAX_WASM_FUNCTIONS as usize {
                bail!("found too many hints");
            }
            if func.hints.count() >= MAX_WASM_FUNCTION_SIZE {
                bail!("found too many hints");
            }
            let hints = func
                .hints
                .into_iter_with_offsets()
                .collect::<wasmparser::Result<Vec<_>>>()?;
            self.code_section_hints.push((func.func, hints));
        }
        self.code_section_hints.reverse();
        Ok(())
    }
}

struct NamedLocalPrinter {
    group_name: &'static str,
    in_group: bool,
    end_group_after_local: bool,
    first: bool,
}

impl NamedLocalPrinter {
    fn new(group_name: &'static str) -> NamedLocalPrinter {
        NamedLocalPrinter {
            group_name,
            in_group: false,
            end_group_after_local: false,
            first: true,
        }
    }

    fn start_local(
        &mut self,
        func: u32,
        local: u32,
        dst: &mut Printer,
        state: &State,
    ) -> Result<()> {
        let name = state.core.local_names.index_to_name.get(&(func, local));

        // Named locals must be in their own group, so if we have a name we need
        // to terminate the previous group.
        if name.is_some() && self.in_group {
            dst.end_group()?;
            self.in_group = false;
        }

        if self.first {
            self.first = false;
        } else {
            dst.result.write_str(" ")?;
        }

        // Next we either need a separator if we're already in a group or we
        // need to open a group for our new local.
        if !self.in_group {
            dst.start_group(self.group_name)?;
            dst.result.write_str(" ")?;
            self.in_group = true;
        }

        // Print the optional name if given...
        match name {
            Some(name) => {
                name.write(dst)?;
                dst.result.write_str(" ")?;
                self.end_group_after_local = true;
            }
            None if dst.config.name_unnamed => {
                write!(dst.result, "$#local{local} ")?;
                self.end_group_after_local = true;
            }
            None => {
                self.end_group_after_local = false;
            }
        }
        Ok(())
    }

    fn end_local(&mut self, dst: &mut Printer) -> Result<()> {
        if self.end_group_after_local {
            dst.end_group()?;
            self.end_group_after_local = false;
            self.in_group = false;
        }
        Ok(())
    }
    fn finish(self, dst: &mut Printer) -> Result<()> {
        if self.in_group {
            dst.end_group()?;
        }
        Ok(())
    }
}

macro_rules! print_float {
    ($name:ident $float:ident $uint:ident $sint:ident $exp_bits:tt) => {
        fn $name(&mut self, mut bits: $uint) -> Result<()> {
            // Calculate a few constants
            let int_width = mem::size_of::<$uint>() * 8;
            let exp_width = $exp_bits;
            let mantissa_width = int_width - 1 - exp_width;
            let bias = (1 << (exp_width - 1)) - 1;
            let max_exp = (1 as $sint) << (exp_width - 1);
            let min_exp = -max_exp + 1;

            // Handle `NaN` and infinity specially
            let f = $float::from_bits(bits);
            if bits >> (int_width - 1) != 0 {
                bits ^= 1 << (int_width - 1);
                self.result.write_str("-")?;
            }
            if f.is_infinite() {
                self.result.start_literal()?;
                self.result.write_str("inf ")?;
                self.result.start_comment()?;
                write!(self.result, "(;={f};)")?;
                self.result.reset_color()?;
                return Ok(());
            }
            if f.is_nan() {
                let payload = bits & ((1 << mantissa_width) - 1);
                self.result.start_literal()?;
                if payload == 1 << (mantissa_width - 1) {
                    self.result.write_str("nan ")?;
                    self.result.start_comment()?;
                    write!(self.result, "(;={f};)")?;
                } else {
                    write!(self.result, "nan:{:#x} ", payload)?;
                    self.result.start_comment()?;
                    write!(self.result, "(;={f};)")?;
                }
                self.result.reset_color()?;
                return Ok(());
            }

            // Figure out our exponent, but keep in mine that it's in an
            // integer width that may not be supported. As a result we do a few
            // tricks here:
            //
            // * Make the MSB the top bit of the exponent, then shift the
            //   exponent to the bottom. This means we now have a signed
            //   integer in `$sint` width representing the whole exponent.
            // * Do the arithmetic for the exponent (subtract)
            // * Next we only care about the lowest `$exp_bits` bits of the
            //   result, but we do care about the sign. Use sign-carrying of
            //   the signed integer shifts to shift it left then shift it back.
            //
            // Overall this should do basic arithmetic for `$exp_bits` bit
            // numbers and get the result back as a signed integer with `$sint`
            // bits in `exponent` representing the same decimal value.
            let mut exponent = (((bits << 1) as $sint) >> (mantissa_width + 1)).wrapping_sub(bias);
            exponent = (exponent << (int_width - exp_width)) >> (int_width - exp_width);
            let mut fraction = bits & ((1 << mantissa_width) - 1);
            self.result.start_literal()?;
            self.result.write_str("0x")?;
            if bits == 0 {
                self.result.write_str("0p+0")?;
            } else {
                self.result.write_str("1")?;
                if fraction > 0 {
                    fraction <<= (int_width - mantissa_width);

                    // Apparently the subnormal is handled here. I don't know
                    // what a subnormal is. If someone else does, please let me
                    // know!
                    if exponent == min_exp {
                        let leading = fraction.leading_zeros();
                        if (leading as usize) < int_width - 1 {
                            fraction <<= leading + 1;
                        } else {
                            fraction = 0;
                        }
                        exponent -= leading as $sint;
                    }

                    self.result.write_str(".")?;
                    while fraction > 0 {
                        write!(self.result, "{:x}", fraction >> (int_width - 4))?;
                        fraction <<= 4;
                    }
                }
                write!(self.result, "p{:+}", exponent)?;
            }
            self.result.start_comment()?;
            write!(self.result, " (;={};)", f)?;
            self.result.reset_color()?;
            Ok(())
        }
    };
}

impl Printer<'_, '_> {
    print_float!(print_f32 f32 u32 i32 8);
    print_float!(print_f64 f64 u64 i64 11);
}

impl Naming {
    fn new<'a>(
        name: &'a str,
        index: u32,
        group: &str,
        used: Option<&mut HashSet<&'a str>>,
    ) -> Naming {
        let mut kind = NamingKind::DollarName;
        if name.chars().any(|c| !is_idchar(c)) {
            kind = NamingKind::DollarQuotedName;
        }

        // If the `name` provided can't be used as the raw identifier for the
        // item that it's describing then a synthetic name must be made. The
        // rules here which generate a name are:
        //
        // * Empty identifiers are not allowed
        // * Identifiers have a fixed set of valid characters
        // * For wasmprinter's purposes we "reserve" identifiers with the `#`
        //   prefix, which is in theory rare to encounter in practice.
        // * If the name has already been used for some other item and cannot
        //   be reused (e.g. because shadowing in this context is not possible).
        //
        // If any of these conditions match then we generate a unique identifier
        // based on `name` but not it exactly. By factoring in the `group`,
        // `index`, and `name` we get a guaranteed unique identifier (due to the
        // leading `#` prefix that we reserve and factoring in of the item
        // index) while preserving human readability at least somewhat (the
        // valid identifier characters of `name` still appear in the returned
        // name).
        if name.is_empty()
            || name.starts_with('#')
            || used.map(|set| !set.insert(name)).unwrap_or(false)
        {
            kind = NamingKind::SyntheticPrefix(format!("#{group}{index}"));
        }
        return Naming {
            kind,
            name: name.to_string(),
        };

        // See https://webassembly.github.io/spec/core/text/values.html#text-id
        fn is_idchar(c: char) -> bool {
            matches!(
                c,
                '0'..='9'
                | 'a'..='z'
                | 'A'..='Z'
                | '!'
                | '#'
                | '$'
                | '%'
                | '&'
                | '\''
                | '*'
                | '+'
                | '-'
                | '.'
                | '/'
                | ':'
                | '<'
                | '='
                | '>'
                | '?'
                | '@'
                | '\\'
                | '^'
                | '_'
                | '`'
                | '|'
                | '~'
            )
        }
    }

    fn write_identifier(&self, printer: &mut Printer<'_, '_>) -> Result<()> {
        match &self.kind {
            NamingKind::DollarName => {
                printer.result.write_str("$")?;
                printer.result.write_str(&self.name)?;
            }
            NamingKind::DollarQuotedName => {
                printer.result.write_str("$\"")?;
                printer.print_str_contents(&self.name)?;
                printer.result.write_str("\"")?;
            }
            NamingKind::SyntheticPrefix(prefix) => {
                printer.result.write_str("$\"")?;
                printer.result.write_str(&prefix)?;
                printer.result.write_str(" ")?;
                printer.print_str_contents(&self.name)?;
                printer.result.write_str("\"")?;
            }
        }
        Ok(())
    }

    fn write(&self, dst: &mut Printer<'_, '_>) -> Result<()> {
        self.write_identifier(dst)?;
        match &self.kind {
            NamingKind::DollarName | NamingKind::DollarQuotedName => {}

            NamingKind::SyntheticPrefix(_) => {
                dst.result.write_str(" ")?;
                dst.start_group("@name \"")?;
                dst.print_str_contents(&self.name)?;
                dst.result.write_str("\"")?;
                dst.end_group()?;
            }
        }
        Ok(())
    }
}

/// Helper trait for the `NamingMap` type's `K` type parameter.
trait NamingNamespace {
    fn desc() -> &'static str;
}

macro_rules! naming_namespaces {
    ($(struct $name:ident => $desc:tt)*) => ($(
        struct $name;

        impl NamingNamespace for $name {
            fn desc() -> &'static str { $desc }
        }
    )*)
}

naming_namespaces! {
    struct NameFunc => "func"
    struct NameModule => "module"
    struct NameInstance => "instance"
    struct NameGlobal => "global"
    struct NameMemory => "memory"
    struct NameLocal => "local"
    struct NameLabel => "label"
    struct NameTable => "table"
    struct NameValue => "value"
    struct NameType => "type"
    struct NameField => "field"
    struct NameData => "data"
    struct NameElem => "elem"
    struct NameComponent => "component"
    struct NameTag => "tag"
}

fn name_map<K>(into: &mut NamingMap<u32, K>, names: NameMap<'_>, name: &str) -> Result<()> {
    let mut used = HashSet::new();
    for naming in names {
        let naming = naming?;
        into.index_to_name.insert(
            naming.index,
            Naming::new(naming.name, naming.index, name, Some(&mut used)),
        );
    }
    Ok(())
}
