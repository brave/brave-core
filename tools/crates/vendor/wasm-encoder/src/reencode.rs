//! Conversions from `wasmparser` to `wasm-encoder` to [`Reencode`] parsed wasm.
//!
//! The [`RoundtripReencoder`] allows encoding identical wasm to the parsed
//! input.

use std::convert::Infallible;

#[allow(missing_docs)] // FIXME
pub trait Reencode {
    type Error;

    fn data_index(&mut self, data: u32) -> u32 {
        utils::data_index(self, data)
    }

    fn element_index(&mut self, element: u32) -> u32 {
        utils::element_index(self, element)
    }

    fn function_index(&mut self, func: u32) -> u32 {
        utils::function_index(self, func)
    }

    fn global_index(&mut self, global: u32) -> u32 {
        utils::global_index(self, global)
    }

    fn memory_index(&mut self, memory: u32) -> u32 {
        utils::memory_index(self, memory)
    }

    fn table_index(&mut self, table: u32) -> u32 {
        utils::table_index(self, table)
    }

    fn tag_index(&mut self, tag: u32) -> u32 {
        utils::tag_index(self, tag)
    }

    fn type_index(&mut self, ty: u32) -> u32 {
        utils::type_index(self, ty)
    }

    fn component_type_index(&mut self, ty: u32) -> u32 {
        utils::component_type_index(self, ty)
    }

    fn abstract_heap_type(
        &mut self,
        value: wasmparser::AbstractHeapType,
    ) -> crate::AbstractHeapType {
        utils::abstract_heap_type(self, value)
    }

    fn array_type(
        &mut self,
        array_ty: wasmparser::ArrayType,
    ) -> Result<crate::ArrayType, Error<Self::Error>> {
        utils::array_type(self, array_ty)
    }

    fn block_type(
        &mut self,
        arg: wasmparser::BlockType,
    ) -> Result<crate::BlockType, Error<Self::Error>> {
        utils::block_type(self, arg)
    }

    fn component_primitive_val_type(
        &mut self,
        ty: wasmparser::PrimitiveValType,
    ) -> crate::component::PrimitiveValType {
        utils::component_primitive_val_type(self, ty)
    }

    fn component_export_kind(
        &mut self,
        ty: wasmparser::ComponentExternalKind,
    ) -> crate::component::ComponentExportKind {
        utils::component_export_kind(self, ty)
    }

    fn component_outer_alias_kind(
        &mut self,
        kind: wasmparser::ComponentOuterAliasKind,
    ) -> crate::component::ComponentOuterAliasKind {
        utils::component_outer_alias_kind(self, kind)
    }

    fn component_val_type(
        &mut self,
        ty: wasmparser::ComponentValType,
    ) -> crate::component::ComponentValType {
        utils::component_val_type(self, ty)
    }

    fn type_bounds(&mut self, ty: wasmparser::TypeBounds) -> crate::component::TypeBounds {
        utils::type_bounds(self, ty)
    }

    fn canonical_option(
        &mut self,
        ty: wasmparser::CanonicalOption,
    ) -> crate::component::CanonicalOption {
        utils::canonical_option(self, ty)
    }

    fn component_type_ref(
        &mut self,
        ty: wasmparser::ComponentTypeRef,
    ) -> crate::component::ComponentTypeRef {
        utils::component_type_ref(self, ty)
    }

    fn const_expr(
        &mut self,
        const_expr: wasmparser::ConstExpr,
    ) -> Result<crate::ConstExpr, Error<Self::Error>> {
        utils::const_expr(self, const_expr)
    }

    fn catch(&mut self, arg: wasmparser::Catch) -> crate::Catch {
        utils::catch(self, arg)
    }

    fn composite_type(
        &mut self,
        composite_ty: wasmparser::CompositeType,
    ) -> Result<crate::CompositeType, Error<Self::Error>> {
        utils::composite_type(self, composite_ty)
    }

    fn entity_type(
        &mut self,
        type_ref: wasmparser::TypeRef,
    ) -> Result<crate::EntityType, Error<Self::Error>> {
        utils::entity_type(self, type_ref)
    }

    fn export_kind(&mut self, external_kind: wasmparser::ExternalKind) -> crate::ExportKind {
        utils::export_kind(self, external_kind)
    }

    fn field_type(
        &mut self,
        field_ty: wasmparser::FieldType,
    ) -> Result<crate::FieldType, Error<Self::Error>> {
        utils::field_type(self, field_ty)
    }

    fn func_type(
        &mut self,
        func_ty: wasmparser::FuncType,
    ) -> Result<crate::FuncType, Error<Self::Error>> {
        utils::func_type(self, func_ty)
    }

    fn global_type(
        &mut self,
        global_ty: wasmparser::GlobalType,
    ) -> Result<crate::GlobalType, Error<Self::Error>> {
        utils::global_type(self, global_ty)
    }

    fn heap_type(
        &mut self,
        heap_type: wasmparser::HeapType,
    ) -> Result<crate::HeapType, Error<Self::Error>> {
        utils::heap_type(self, heap_type)
    }

    fn instruction<'a>(
        &mut self,
        arg: wasmparser::Operator<'a>,
    ) -> Result<crate::Instruction<'a>, Error<Self::Error>> {
        utils::instruction(self, arg)
    }

    fn memory_type(&mut self, memory_ty: wasmparser::MemoryType) -> crate::MemoryType {
        utils::memory_type(self, memory_ty)
    }

    fn mem_arg(&mut self, arg: wasmparser::MemArg) -> crate::MemArg {
        utils::mem_arg(self, arg)
    }

    fn ordering(&mut self, arg: wasmparser::Ordering) -> crate::Ordering {
        utils::ordering(self, arg)
    }

    fn ref_type(
        &mut self,
        ref_type: wasmparser::RefType,
    ) -> Result<crate::RefType, Error<Self::Error>> {
        utils::ref_type(self, ref_type)
    }

    fn storage_type(
        &mut self,
        storage_ty: wasmparser::StorageType,
    ) -> Result<crate::StorageType, Error<Self::Error>> {
        utils::storage_type(self, storage_ty)
    }

    fn struct_type(
        &mut self,
        struct_ty: wasmparser::StructType,
    ) -> Result<crate::StructType, Error<Self::Error>> {
        utils::struct_type(self, struct_ty)
    }

    fn sub_type(
        &mut self,
        sub_ty: wasmparser::SubType,
    ) -> Result<crate::SubType, Error<Self::Error>> {
        utils::sub_type(self, sub_ty)
    }

    fn table_type(
        &mut self,
        table_ty: wasmparser::TableType,
    ) -> Result<crate::TableType, Error<Self::Error>> {
        utils::table_type(self, table_ty)
    }

    fn tag_kind(&mut self, kind: wasmparser::TagKind) -> crate::TagKind {
        utils::tag_kind(self, kind)
    }

    fn tag_type(&mut self, tag_ty: wasmparser::TagType) -> crate::TagType {
        utils::tag_type(self, tag_ty)
    }

    fn val_type(
        &mut self,
        val_ty: wasmparser::ValType,
    ) -> Result<crate::ValType, Error<Self::Error>> {
        utils::val_type(self, val_ty)
    }

    /// Parses the input `section` given from the `wasmparser` crate and
    /// adds the custom section to the `module`.
    fn parse_custom_section(
        &mut self,
        module: &mut crate::Module,
        section: wasmparser::CustomSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_custom_section(self, module, section)
    }

    /// Converts the input `section` given from the `wasmparser` crate into an
    /// encoded custom section.
    fn custom_section<'a>(
        &mut self,
        section: wasmparser::CustomSectionReader<'a>,
    ) -> crate::CustomSection<'a> {
        utils::custom_section(self, section)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the code to the `code` section.
    fn parse_code_section(
        &mut self,
        code: &mut crate::CodeSection,
        section: wasmparser::CodeSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_code_section(self, code, section)
    }

    /// Parses a single [`wasmparser::FunctionBody`] and adds it to the `code` section.
    fn parse_function_body(
        &mut self,
        code: &mut crate::CodeSection,
        func: wasmparser::FunctionBody<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_function_body(self, code, func)
    }

    /// Create a new [`crate::Function`] by parsing the locals declarations from the
    /// provided [`wasmparser::FunctionBody`].
    fn new_function_with_parsed_locals(
        &mut self,
        func: &wasmparser::FunctionBody<'_>,
    ) -> Result<crate::Function, Error<Self::Error>> {
        utils::new_function_with_parsed_locals(self, func)
    }

    /// Parses a single instruction from `reader` and adds it to `function`.
    fn parse_instruction(
        &mut self,
        function: &mut crate::Function,
        reader: &mut wasmparser::OperatorsReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_instruction(self, function, reader)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the data to the `data` section.
    fn parse_data_section(
        &mut self,
        data: &mut crate::DataSection,
        section: wasmparser::DataSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_data_section(self, data, section)
    }

    /// Parses a single [`wasmparser::Data`] and adds it to the `data` section.
    fn parse_data(
        &mut self,
        data: &mut crate::DataSection,
        datum: wasmparser::Data<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_data(self, data, datum)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the elements to the `element` section.
    fn parse_element_section(
        &mut self,
        elements: &mut crate::ElementSection,
        section: wasmparser::ElementSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_element_section(self, elements, section)
    }

    /// Parses the single [`wasmparser::Element`] provided and adds it to the
    /// `element` section.
    fn parse_element(
        &mut self,
        elements: &mut crate::ElementSection,
        element: wasmparser::Element<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_element(self, elements, element)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the exports to the `exports` section.
    fn parse_export_section(
        &mut self,
        exports: &mut crate::ExportSection,
        section: wasmparser::ExportSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_export_section(self, exports, section)
    }

    /// Parses the single [`wasmparser::Export`] provided and adds it to the
    /// `exports` section.
    fn parse_export(&mut self, exports: &mut crate::ExportSection, export: wasmparser::Export<'_>) {
        utils::parse_export(self, exports, export)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the functions to the `functions` section.
    fn parse_function_section(
        &mut self,
        functions: &mut crate::FunctionSection,
        section: wasmparser::FunctionSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_function_section(self, functions, section)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the globals to the `globals` section.
    fn parse_global_section(
        &mut self,
        globals: &mut crate::GlobalSection,
        section: wasmparser::GlobalSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_global_section(self, globals, section)
    }

    /// Parses the single [`wasmparser::Global`] provided and adds it to the
    /// `globals` section.
    fn parse_global(
        &mut self,
        globals: &mut crate::GlobalSection,
        global: wasmparser::Global<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_global(self, globals, global)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the imports to the `import` section.
    fn parse_import_section(
        &mut self,
        imports: &mut crate::ImportSection,
        section: wasmparser::ImportSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_import_section(self, imports, section)
    }

    /// Parses the single [`wasmparser::Import`] provided and adds it to the
    /// `import` section.
    fn parse_import(
        &mut self,
        imports: &mut crate::ImportSection,
        import: wasmparser::Import<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_import(self, imports, import)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the memories to the `memories` section.
    fn parse_memory_section(
        &mut self,
        memories: &mut crate::MemorySection,
        section: wasmparser::MemorySectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_memory_section(self, memories, section)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the tables to the `tables` section.
    fn parse_table_section(
        &mut self,
        tables: &mut crate::TableSection,
        section: wasmparser::TableSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_table_section(self, tables, section)
    }

    /// Parses a single [`wasmparser::Table`] and adds it to the `tables` section.
    fn parse_table(
        &mut self,
        tables: &mut crate::TableSection,
        table: wasmparser::Table<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_table(self, tables, table)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the tags to the `tags` section.
    fn parse_tag_section(
        &mut self,
        tags: &mut crate::TagSection,
        section: wasmparser::TagSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_tag_section(self, tags, section)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the types to the `types` section.
    fn parse_type_section(
        &mut self,
        types: &mut crate::TypeSection,
        section: wasmparser::TypeSectionReader<'_>,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_type_section(self, types, section)
    }

    /// Parses a single [`wasmparser::RecGroup`] and adds it to the `types` section.
    fn parse_recursive_type_group(
        &mut self,
        types: &mut crate::TypeSection,
        rec_group: wasmparser::RecGroup,
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_recursive_type_group(self, types, rec_group)
    }

    fn parse_unknown_section(
        &mut self,
        module: &mut crate::Module,
        id: u8,
        contents: &[u8],
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_unknown_section(self, module, id, contents)
    }

    /// A hook method that is called inside [`Reencode::parse_core_module`]
    /// before and after every non-custom core wasm section.
    ///
    /// This method can be used to insert new custom sections in between those
    /// sections, or to detect when a non-custom section is missing and insert
    /// it in the [proper order].
    ///
    /// The `after` parameter is `None` iff the hook is called before the first
    /// non-custom section, and `Some(s)` afterwards, where `s` is the
    /// [`SectionId`] of the previous non-custom section.
    ///
    /// The `before` parameter is `None` iff the hook is called after the last
    /// non-custom section, and `Some(s)` beforehand, where `s` is the
    /// [`SectionId`] of the following non-custom section.
    ///
    /// [proper order]: https://webassembly.github.io/spec/core/binary/modules.html#binary-module
    /// [`SectionId`]: crate::SectionId
    fn intersperse_section_hook(
        &mut self,
        module: &mut crate::Module,
        after: Option<crate::SectionId>,
        before: Option<crate::SectionId>,
    ) -> Result<(), Error<Self::Error>> {
        utils::intersperse_section_hook(self, module, after, before)
    }

    fn parse_core_module(
        &mut self,
        module: &mut crate::Module,
        parser: wasmparser::Parser,
        data: &[u8],
    ) -> Result<(), Error<Self::Error>> {
        utils::parse_core_module(self, module, parser, data)
    }
}

/// An error when re-encoding from `wasmparser` to `wasm-encoder`.
#[derive(Debug)]
pub enum Error<E = Infallible> {
    /// There was a type reference that was canonicalized and no longer
    /// references an index into a module's types space, so we cannot encode it
    /// into a Wasm binary again.
    CanonicalizedHeapTypeReference,
    /// The const expression is invalid: not actually constant or something like
    /// that.
    InvalidConstExpr,
    /// There was a section that does not belong into a core wasm module.
    UnexpectedNonCoreModuleSection,
    /// There was an error when parsing.
    ParseError(wasmparser::BinaryReaderError),
    /// There was a user-defined error when re-encoding.
    UserError(E),
}

impl<E> From<wasmparser::BinaryReaderError> for Error<E> {
    fn from(err: wasmparser::BinaryReaderError) -> Self {
        Self::ParseError(err)
    }
}

impl<E: std::fmt::Display> std::fmt::Display for Error<E> {
    fn fmt(&self, fmt: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Self::ParseError(_e) => {
                write!(fmt, "There was an error when parsing")
            }
            Self::UserError(e) => write!(fmt, "{e}"),
            Self::InvalidConstExpr => write!(fmt, "The const expression was invalid"),
            Self::UnexpectedNonCoreModuleSection => write!(
                fmt,
                "There was a section that does not belong into a core wasm module"
            ),
            Self::CanonicalizedHeapTypeReference => write!(
                fmt,
                "There was a canonicalized heap type reference without type index information"
            ),
        }
    }
}

impl<E: 'static + std::error::Error> std::error::Error for Error<E> {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Self::ParseError(e) => Some(e),
            Self::UserError(e) => Some(e),
            Self::InvalidConstExpr
            | Self::CanonicalizedHeapTypeReference
            | Self::UnexpectedNonCoreModuleSection => None,
        }
    }
}

/// Reencodes `wasmparser` into `wasm-encoder` so that the encoded wasm is
/// identical to the input and can be parsed and encoded again.
#[derive(Debug)]
pub struct RoundtripReencoder;

impl Reencode for RoundtripReencoder {
    type Error = Infallible;
}

#[allow(missing_docs)] // FIXME
pub mod utils {
    use super::{Error, Reencode};

    pub fn parse_core_module<T: ?Sized + Reencode>(
        reencoder: &mut T,
        module: &mut crate::Module,
        parser: wasmparser::Parser,
        data: &[u8],
    ) -> Result<(), Error<T::Error>> {
        fn handle_intersperse_section_hook<T: ?Sized + Reencode>(
            reencoder: &mut T,
            module: &mut crate::Module,
            last_section: &mut Option<crate::SectionId>,
            next_section: Option<crate::SectionId>,
        ) -> Result<(), Error<T::Error>> {
            let after = std::mem::replace(last_section, next_section.clone());
            let before = next_section;
            reencoder.intersperse_section_hook(module, after, before)
        }

        let mut sections = parser.parse_all(data);
        let mut next_section = sections.next();
        let mut last_section = None;

        'outer: while let Some(section) = next_section {
            match section? {
                wasmparser::Payload::Version {
                    encoding: wasmparser::Encoding::Module,
                    ..
                } => (),
                wasmparser::Payload::Version { .. } => {
                    return Err(Error::UnexpectedNonCoreModuleSection)
                }
                wasmparser::Payload::TypeSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Type),
                    )?;
                    let mut types = crate::TypeSection::new();
                    reencoder.parse_type_section(&mut types, section)?;
                    module.section(&types);
                }
                wasmparser::Payload::ImportSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Import),
                    )?;
                    let mut imports = crate::ImportSection::new();
                    reencoder.parse_import_section(&mut imports, section)?;
                    module.section(&imports);
                }
                wasmparser::Payload::FunctionSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Function),
                    )?;
                    let mut functions = crate::FunctionSection::new();
                    reencoder.parse_function_section(&mut functions, section)?;
                    module.section(&functions);
                }
                wasmparser::Payload::TableSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Table),
                    )?;
                    let mut tables = crate::TableSection::new();
                    reencoder.parse_table_section(&mut tables, section)?;
                    module.section(&tables);
                }
                wasmparser::Payload::MemorySection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Memory),
                    )?;
                    let mut memories = crate::MemorySection::new();
                    reencoder.parse_memory_section(&mut memories, section)?;
                    module.section(&memories);
                }
                wasmparser::Payload::TagSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Tag),
                    )?;
                    let mut tags = crate::TagSection::new();
                    reencoder.parse_tag_section(&mut tags, section)?;
                    module.section(&tags);
                }
                wasmparser::Payload::GlobalSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Global),
                    )?;
                    let mut globals = crate::GlobalSection::new();
                    reencoder.parse_global_section(&mut globals, section)?;
                    module.section(&globals);
                }
                wasmparser::Payload::ExportSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Export),
                    )?;
                    let mut exports = crate::ExportSection::new();
                    reencoder.parse_export_section(&mut exports, section)?;
                    module.section(&exports);
                }
                wasmparser::Payload::StartSection { func, .. } => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Start),
                    )?;
                    module.section(&crate::StartSection {
                        function_index: reencoder.function_index(func),
                    });
                }
                wasmparser::Payload::ElementSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Element),
                    )?;
                    let mut elements = crate::ElementSection::new();
                    reencoder.parse_element_section(&mut elements, section)?;
                    module.section(&elements);
                }
                wasmparser::Payload::DataCountSection { count, .. } => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::DataCount),
                    )?;
                    module.section(&crate::DataCountSection { count });
                }
                wasmparser::Payload::DataSection(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Data),
                    )?;
                    let mut data = crate::DataSection::new();
                    reencoder.parse_data_section(&mut data, section)?;
                    module.section(&data);
                }
                wasmparser::Payload::CodeSectionStart { count, .. } => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Code),
                    )?;
                    let mut codes = crate::CodeSection::new();
                    for _ in 0..count {
                        if let Some(Ok(wasmparser::Payload::CodeSectionEntry(section))) =
                            sections.next()
                        {
                            reencoder.parse_function_body(&mut codes, section)?;
                        } else {
                            return Err(Error::UnexpectedNonCoreModuleSection);
                        }
                    }
                    module.section(&codes);
                }
                wasmparser::Payload::CodeSectionEntry(section) => {
                    handle_intersperse_section_hook(
                        reencoder,
                        module,
                        &mut last_section,
                        Some(crate::SectionId::Code),
                    )?;
                    // we can't do better than start a new code section here
                    let mut codes = crate::CodeSection::new();
                    reencoder.parse_function_body(&mut codes, section)?;
                    while let Some(section) = sections.next() {
                        let section = section?;
                        if let wasmparser::Payload::CodeSectionEntry(section) = section {
                            reencoder.parse_function_body(&mut codes, section)?;
                        } else {
                            module.section(&codes);
                            next_section = Some(Ok(section));
                            continue 'outer;
                        }
                    }
                    module.section(&codes);
                }
                wasmparser::Payload::ModuleSection { .. }
                | wasmparser::Payload::InstanceSection(_)
                | wasmparser::Payload::CoreTypeSection(_)
                | wasmparser::Payload::ComponentSection { .. }
                | wasmparser::Payload::ComponentInstanceSection(_)
                | wasmparser::Payload::ComponentAliasSection(_)
                | wasmparser::Payload::ComponentTypeSection(_)
                | wasmparser::Payload::ComponentCanonicalSection(_)
                | wasmparser::Payload::ComponentStartSection { .. }
                | wasmparser::Payload::ComponentImportSection(_)
                | wasmparser::Payload::ComponentExportSection(_) => {
                    return Err(Error::UnexpectedNonCoreModuleSection)
                }
                wasmparser::Payload::CustomSection(section) => {
                    reencoder.parse_custom_section(module, section)?;
                }
                wasmparser::Payload::UnknownSection { id, contents, .. } => {
                    reencoder.parse_unknown_section(module, id, contents)?;
                }
                wasmparser::Payload::End(_) => {
                    handle_intersperse_section_hook(reencoder, module, &mut last_section, None)?;
                }
            }

            next_section = sections.next();
        }

        Ok(())
    }

    /// A hook method that is called inside [`Reencode::parse_core_module`]
    /// before and after every non-custom core wasm section.
    ///
    /// This method can be used to insert new custom sections in between those
    /// sections, or to detect when a non-custom section is missing and insert
    /// it in the [proper order].
    ///
    /// The `after` parameter is `None` iff the hook is called before the first
    /// non-custom section, and `Some(s)` afterwards, where `s` is the
    /// [`SectionId`] of the previous non-custom section.
    ///
    /// The `before` parameter is `None` iff the hook is called after the last
    /// non-custom section, and `Some(s)` beforehand, where `s` is the
    /// [`SectionId`] of the following non-custom section.
    ///
    /// [proper order]: https://webassembly.github.io/spec/core/binary/modules.html#binary-module
    /// [`SectionId`]: crate::SectionId
    pub fn intersperse_section_hook<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        _module: &mut crate::Module,
        _after: Option<crate::SectionId>,
        _before: Option<crate::SectionId>,
    ) -> Result<(), Error<T::Error>> {
        Ok(())
    }

    pub fn component_primitive_val_type<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        ty: wasmparser::PrimitiveValType,
    ) -> crate::component::PrimitiveValType {
        match ty {
            wasmparser::PrimitiveValType::Bool => crate::component::PrimitiveValType::Bool,
            wasmparser::PrimitiveValType::S8 => crate::component::PrimitiveValType::S8,
            wasmparser::PrimitiveValType::U8 => crate::component::PrimitiveValType::U8,
            wasmparser::PrimitiveValType::S16 => crate::component::PrimitiveValType::S16,
            wasmparser::PrimitiveValType::U16 => crate::component::PrimitiveValType::U16,
            wasmparser::PrimitiveValType::S32 => crate::component::PrimitiveValType::S32,
            wasmparser::PrimitiveValType::U32 => crate::component::PrimitiveValType::U32,
            wasmparser::PrimitiveValType::S64 => crate::component::PrimitiveValType::S64,
            wasmparser::PrimitiveValType::U64 => crate::component::PrimitiveValType::U64,
            wasmparser::PrimitiveValType::F32 => crate::component::PrimitiveValType::F32,
            wasmparser::PrimitiveValType::F64 => crate::component::PrimitiveValType::F64,
            wasmparser::PrimitiveValType::Char => crate::component::PrimitiveValType::Char,
            wasmparser::PrimitiveValType::String => crate::component::PrimitiveValType::String,
        }
    }

    pub fn component_export_kind<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        ty: wasmparser::ComponentExternalKind,
    ) -> crate::component::ComponentExportKind {
        match ty {
            wasmparser::ComponentExternalKind::Module => crate::ComponentExportKind::Module,
            wasmparser::ComponentExternalKind::Func => crate::ComponentExportKind::Func,
            wasmparser::ComponentExternalKind::Value => crate::ComponentExportKind::Value,
            wasmparser::ComponentExternalKind::Type => crate::ComponentExportKind::Type,
            wasmparser::ComponentExternalKind::Instance => crate::ComponentExportKind::Instance,
            wasmparser::ComponentExternalKind::Component => crate::ComponentExportKind::Component,
        }
    }

    pub fn component_outer_alias_kind<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        ty: wasmparser::ComponentOuterAliasKind,
    ) -> crate::component::ComponentOuterAliasKind {
        match ty {
            wasmparser::ComponentOuterAliasKind::CoreModule => {
                crate::component::ComponentOuterAliasKind::CoreModule
            }
            wasmparser::ComponentOuterAliasKind::CoreType => {
                crate::component::ComponentOuterAliasKind::CoreType
            }
            wasmparser::ComponentOuterAliasKind::Type => {
                crate::component::ComponentOuterAliasKind::Type
            }
            wasmparser::ComponentOuterAliasKind::Component => {
                crate::ComponentOuterAliasKind::Component
            }
        }
    }

    pub fn component_val_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        ty: wasmparser::ComponentValType,
    ) -> crate::component::ComponentValType {
        match ty {
            wasmparser::ComponentValType::Type(u) => {
                crate::component::ComponentValType::Type(reencoder.component_type_index(u))
            }
            wasmparser::ComponentValType::Primitive(pty) => {
                crate::component::ComponentValType::Primitive(
                    crate::component::PrimitiveValType::from(pty),
                )
            }
        }
    }

    pub fn type_bounds<T: ?Sized + Reencode>(
        reencoder: &mut T,
        ty: wasmparser::TypeBounds,
    ) -> crate::component::TypeBounds {
        match ty {
            wasmparser::TypeBounds::Eq(u) => {
                crate::component::TypeBounds::Eq(reencoder.component_type_index(u))
            }
            wasmparser::TypeBounds::SubResource => crate::component::TypeBounds::SubResource,
        }
    }

    pub fn component_type_ref<T: ?Sized + Reencode>(
        reencoder: &mut T,
        ty: wasmparser::ComponentTypeRef,
    ) -> crate::component::ComponentTypeRef {
        match ty {
            wasmparser::ComponentTypeRef::Module(u) => {
                crate::component::ComponentTypeRef::Module(reencoder.component_type_index(u))
            }
            wasmparser::ComponentTypeRef::Func(u) => {
                crate::component::ComponentTypeRef::Func(reencoder.component_type_index(u))
            }
            wasmparser::ComponentTypeRef::Value(valty) => {
                crate::component::ComponentTypeRef::Value(reencoder.component_val_type(valty))
            }
            wasmparser::ComponentTypeRef::Type(bounds) => {
                crate::component::ComponentTypeRef::Type(reencoder.type_bounds(bounds))
            }
            wasmparser::ComponentTypeRef::Instance(u) => {
                crate::component::ComponentTypeRef::Instance(reencoder.component_type_index(u))
            }
            wasmparser::ComponentTypeRef::Component(u) => {
                crate::component::ComponentTypeRef::Component(reencoder.component_type_index(u))
            }
        }
    }

    pub fn canonical_option<T: ?Sized + Reencode>(
        reencoder: &mut T,
        ty: wasmparser::CanonicalOption,
    ) -> crate::component::CanonicalOption {
        match ty {
            wasmparser::CanonicalOption::UTF8 => crate::component::CanonicalOption::UTF8,
            wasmparser::CanonicalOption::UTF16 => crate::component::CanonicalOption::UTF16,
            wasmparser::CanonicalOption::CompactUTF16 => {
                crate::component::CanonicalOption::CompactUTF16
            }
            wasmparser::CanonicalOption::Memory(u) => {
                crate::component::CanonicalOption::Memory(reencoder.memory_index(u))
            }
            wasmparser::CanonicalOption::Realloc(u) => {
                crate::component::CanonicalOption::Realloc(reencoder.function_index(u))
            }
            wasmparser::CanonicalOption::PostReturn(u) => {
                crate::component::CanonicalOption::PostReturn(reencoder.function_index(u))
            }
        }
    }

    pub fn memory_index<T: ?Sized + Reencode>(_reencoder: &mut T, memory: u32) -> u32 {
        memory
    }

    pub fn mem_arg<T: ?Sized + Reencode>(
        reencoder: &mut T,
        arg: wasmparser::MemArg,
    ) -> crate::MemArg {
        crate::MemArg {
            offset: arg.offset,
            align: arg.align.into(),
            memory_index: reencoder.memory_index(arg.memory),
        }
    }

    pub fn ordering<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        arg: wasmparser::Ordering,
    ) -> crate::Ordering {
        match arg {
            wasmparser::Ordering::SeqCst => crate::Ordering::SeqCst,
            wasmparser::Ordering::AcqRel => crate::Ordering::AcqRel,
        }
    }

    pub fn function_index<T: ?Sized + Reencode>(_reencoder: &mut T, func: u32) -> u32 {
        func
    }

    pub fn tag_index<T: ?Sized + Reencode>(_reencoder: &mut T, tag: u32) -> u32 {
        tag
    }

    pub fn catch<T: ?Sized + Reencode>(reencoder: &mut T, arg: wasmparser::Catch) -> crate::Catch {
        match arg {
            wasmparser::Catch::One { tag, label } => crate::Catch::One {
                tag: reencoder.tag_index(tag),
                label,
            },
            wasmparser::Catch::OneRef { tag, label } => crate::Catch::OneRef {
                tag: reencoder.tag_index(tag),
                label,
            },
            wasmparser::Catch::All { label } => crate::Catch::All { label },
            wasmparser::Catch::AllRef { label } => crate::Catch::AllRef { label },
        }
    }

    /// Parses the input `section` given from the `wasmparser` crate and
    /// adds the custom section to the `module`.
    pub fn parse_custom_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        module: &mut crate::Module,
        section: wasmparser::CustomSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        module.section(&reencoder.custom_section(section));
        Ok(())
    }

    /// Converts the input `section` given from the `wasmparser` crate into an
    /// encoded custom section.
    pub fn custom_section<'a, T: ?Sized + Reencode>(
        _reencoder: &mut T,
        section: wasmparser::CustomSectionReader<'a>,
    ) -> crate::CustomSection<'a> {
        crate::CustomSection {
            data: section.data().into(),
            name: section.name().into(),
        }
    }

    pub fn export_kind<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        external_kind: wasmparser::ExternalKind,
    ) -> crate::ExportKind {
        match external_kind {
            wasmparser::ExternalKind::Func => crate::ExportKind::Func,
            wasmparser::ExternalKind::Table => crate::ExportKind::Table,
            wasmparser::ExternalKind::Memory => crate::ExportKind::Memory,
            wasmparser::ExternalKind::Global => crate::ExportKind::Global,
            wasmparser::ExternalKind::Tag => crate::ExportKind::Tag,
        }
    }

    pub fn memory_type<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        memory_ty: wasmparser::MemoryType,
    ) -> crate::MemoryType {
        crate::MemoryType {
            minimum: memory_ty.initial,
            maximum: memory_ty.maximum,
            memory64: memory_ty.memory64,
            shared: memory_ty.shared,
            page_size_log2: memory_ty.page_size_log2,
        }
    }

    pub fn tag_kind<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        kind: wasmparser::TagKind,
    ) -> crate::TagKind {
        match kind {
            wasmparser::TagKind::Exception => crate::TagKind::Exception,
        }
    }

    pub fn type_index<T: ?Sized + Reencode>(_reencoder: &mut T, ty: u32) -> u32 {
        ty
    }

    pub fn component_type_index<T: ?Sized + Reencode>(_reencoder: &mut T, ty: u32) -> u32 {
        ty
    }

    pub fn tag_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        tag_ty: wasmparser::TagType,
    ) -> crate::TagType {
        crate::TagType {
            kind: reencoder.tag_kind(tag_ty.kind),
            func_type_idx: reencoder.type_index(tag_ty.func_type_idx),
        }
    }

    pub fn abstract_heap_type<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        value: wasmparser::AbstractHeapType,
    ) -> crate::AbstractHeapType {
        use wasmparser::AbstractHeapType::*;
        match value {
            Func => crate::AbstractHeapType::Func,
            Extern => crate::AbstractHeapType::Extern,
            Any => crate::AbstractHeapType::Any,
            None => crate::AbstractHeapType::None,
            NoExtern => crate::AbstractHeapType::NoExtern,
            NoFunc => crate::AbstractHeapType::NoFunc,
            Eq => crate::AbstractHeapType::Eq,
            Struct => crate::AbstractHeapType::Struct,
            Array => crate::AbstractHeapType::Array,
            I31 => crate::AbstractHeapType::I31,
            Exn => crate::AbstractHeapType::Exn,
            NoExn => crate::AbstractHeapType::NoExn,
        }
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the types to the `types` section.
    pub fn parse_type_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        types: &mut crate::TypeSection,
        section: wasmparser::TypeSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for rec_group in section {
            reencoder.parse_recursive_type_group(types, rec_group?)?;
        }
        Ok(())
    }

    /// Parses a single [`wasmparser::RecGroup`] and adds it to the `types` section.
    pub fn parse_recursive_type_group<T: ?Sized + Reencode>(
        reencoder: &mut T,
        types: &mut crate::TypeSection,
        rec_group: wasmparser::RecGroup,
    ) -> Result<(), Error<T::Error>> {
        if rec_group.is_explicit_rec_group() {
            let subtypes = rec_group
                .into_types()
                .map(|t| reencoder.sub_type(t))
                .collect::<Result<Vec<_>, _>>()?;
            types.rec(subtypes);
        } else {
            let ty = rec_group.into_types().next().unwrap();
            types.subtype(&reencoder.sub_type(ty)?);
        }
        Ok(())
    }

    pub fn sub_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        sub_ty: wasmparser::SubType,
    ) -> Result<crate::SubType, Error<T::Error>> {
        Ok(crate::SubType {
            is_final: sub_ty.is_final,
            supertype_idx: sub_ty
                .supertype_idx
                .map(|i| {
                    i.as_module_index()
                        .map(|ty| reencoder.type_index(ty))
                        .ok_or(Error::CanonicalizedHeapTypeReference)
                })
                .transpose()?,
            composite_type: reencoder.composite_type(sub_ty.composite_type)?,
        })
    }

    pub fn composite_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        composite_ty: wasmparser::CompositeType,
    ) -> Result<crate::CompositeType, Error<T::Error>> {
        let inner = match composite_ty.inner {
            wasmparser::CompositeInnerType::Func(f) => {
                crate::CompositeInnerType::Func(reencoder.func_type(f)?)
            }
            wasmparser::CompositeInnerType::Array(a) => {
                crate::CompositeInnerType::Array(reencoder.array_type(a)?)
            }
            wasmparser::CompositeInnerType::Struct(s) => {
                crate::CompositeInnerType::Struct(reencoder.struct_type(s)?)
            }
        };
        Ok(crate::CompositeType {
            inner,
            shared: composite_ty.shared,
        })
    }

    pub fn func_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        func_ty: wasmparser::FuncType,
    ) -> Result<crate::FuncType, Error<T::Error>> {
        let mut buf = Vec::with_capacity(func_ty.params().len() + func_ty.results().len());
        for ty in func_ty.params().iter().chain(func_ty.results()).copied() {
            buf.push(reencoder.val_type(ty)?);
        }
        Ok(crate::FuncType::from_parts(
            buf.into(),
            func_ty.params().len(),
        ))
    }

    pub fn array_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        array_ty: wasmparser::ArrayType,
    ) -> Result<crate::ArrayType, Error<T::Error>> {
        Ok(crate::ArrayType(reencoder.field_type(array_ty.0)?))
    }

    pub fn struct_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        struct_ty: wasmparser::StructType,
    ) -> Result<crate::StructType, Error<T::Error>> {
        Ok(crate::StructType {
            fields: struct_ty
                .fields
                .iter()
                .map(|field_ty| reencoder.field_type(field_ty.clone()))
                .collect::<Result<_, _>>()?,
        })
    }

    pub fn field_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        field_ty: wasmparser::FieldType,
    ) -> Result<crate::FieldType, Error<T::Error>> {
        Ok(crate::FieldType {
            element_type: reencoder.storage_type(field_ty.element_type)?,
            mutable: field_ty.mutable,
        })
    }

    pub fn storage_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        storage_ty: wasmparser::StorageType,
    ) -> Result<crate::StorageType, Error<T::Error>> {
        Ok(match storage_ty {
            wasmparser::StorageType::I8 => crate::StorageType::I8,
            wasmparser::StorageType::I16 => crate::StorageType::I16,
            wasmparser::StorageType::Val(v) => crate::StorageType::Val(reencoder.val_type(v)?),
        })
    }

    pub fn val_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        val_ty: wasmparser::ValType,
    ) -> Result<crate::ValType, Error<T::Error>> {
        Ok(match val_ty {
            wasmparser::ValType::I32 => crate::ValType::I32,
            wasmparser::ValType::I64 => crate::ValType::I64,
            wasmparser::ValType::F32 => crate::ValType::F32,
            wasmparser::ValType::F64 => crate::ValType::F64,
            wasmparser::ValType::V128 => crate::ValType::V128,
            wasmparser::ValType::Ref(r) => crate::ValType::Ref(reencoder.ref_type(r)?),
        })
    }

    pub fn ref_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        ref_type: wasmparser::RefType,
    ) -> Result<crate::RefType, Error<T::Error>> {
        Ok(crate::RefType {
            nullable: ref_type.is_nullable(),
            heap_type: reencoder.heap_type(ref_type.heap_type())?,
        })
    }

    pub fn heap_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        heap_type: wasmparser::HeapType,
    ) -> Result<crate::HeapType, Error<T::Error>> {
        Ok(match heap_type {
            wasmparser::HeapType::Concrete(i) => crate::HeapType::Concrete(
                i.as_module_index()
                    .map(|ty| reencoder.type_index(ty))
                    .ok_or(Error::CanonicalizedHeapTypeReference)?,
            ),
            wasmparser::HeapType::Abstract { shared, ty } => crate::HeapType::Abstract {
                shared,
                ty: reencoder.abstract_heap_type(ty),
            },
        })
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the tables to the `tables` section.
    pub fn parse_table_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        tables: &mut crate::TableSection,
        section: wasmparser::TableSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for table in section {
            reencoder.parse_table(tables, table?)?;
        }
        Ok(())
    }

    /// Parses a single [`wasmparser::Table`] and adds it to the `tables` section.
    pub fn parse_table<T: ?Sized + Reencode>(
        reencoder: &mut T,
        tables: &mut crate::TableSection,
        table: wasmparser::Table<'_>,
    ) -> Result<(), Error<T::Error>> {
        let ty = reencoder.table_type(table.ty)?;
        match table.init {
            wasmparser::TableInit::RefNull => {
                tables.table(ty);
            }
            wasmparser::TableInit::Expr(e) => {
                tables.table_with_init(ty, &reencoder.const_expr(e)?);
            }
        }
        Ok(())
    }

    pub fn table_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        table_ty: wasmparser::TableType,
    ) -> Result<crate::TableType, Error<T::Error>> {
        Ok(crate::TableType {
            element_type: reencoder.ref_type(table_ty.element_type)?,
            minimum: table_ty.initial,
            maximum: table_ty.maximum,
            table64: table_ty.table64,
            shared: table_ty.shared,
        })
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the tags to the `tags` section.
    pub fn parse_tag_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        tags: &mut crate::TagSection,
        section: wasmparser::TagSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for tag in section {
            let tag = tag?;
            tags.tag(reencoder.tag_type(tag));
        }
        Ok(())
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the exports to the `exports` section.
    pub fn parse_export_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        exports: &mut crate::ExportSection,
        section: wasmparser::ExportSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for export in section {
            reencoder.parse_export(exports, export?);
        }
        Ok(())
    }

    /// Parses the single [`wasmparser::Export`] provided and adds it to the
    /// `exports` section.
    pub fn parse_export<T: ?Sized + Reencode>(
        reencoder: &mut T,
        exports: &mut crate::ExportSection,
        export: wasmparser::Export<'_>,
    ) {
        exports.export(
            export.name,
            reencoder.export_kind(export.kind),
            match export.kind {
                wasmparser::ExternalKind::Func => reencoder.function_index(export.index),
                wasmparser::ExternalKind::Table => reencoder.table_index(export.index),
                wasmparser::ExternalKind::Memory => reencoder.memory_index(export.index),
                wasmparser::ExternalKind::Global => reencoder.global_index(export.index),
                wasmparser::ExternalKind::Tag => reencoder.tag_index(export.index),
            },
        );
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the globals to the `globals` section.
    pub fn parse_global_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        globals: &mut crate::GlobalSection,
        section: wasmparser::GlobalSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for global in section {
            reencoder.parse_global(globals, global?)?;
        }
        Ok(())
    }

    /// Parses the single [`wasmparser::Global`] provided and adds it to the
    /// `globals` section.
    pub fn parse_global<T: ?Sized + Reencode>(
        reencoder: &mut T,
        globals: &mut crate::GlobalSection,
        global: wasmparser::Global<'_>,
    ) -> Result<(), Error<T::Error>> {
        globals.global(
            reencoder.global_type(global.ty)?,
            &reencoder.const_expr(global.init_expr)?,
        );
        Ok(())
    }

    pub fn global_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        global_ty: wasmparser::GlobalType,
    ) -> Result<crate::GlobalType, Error<T::Error>> {
        Ok(crate::GlobalType {
            val_type: reencoder.val_type(global_ty.content_type)?,
            mutable: global_ty.mutable,
            shared: global_ty.shared,
        })
    }

    pub fn entity_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        type_ref: wasmparser::TypeRef,
    ) -> Result<crate::EntityType, Error<T::Error>> {
        Ok(match type_ref {
            wasmparser::TypeRef::Func(i) => crate::EntityType::Function(reencoder.type_index(i)),
            wasmparser::TypeRef::Table(t) => crate::EntityType::Table(reencoder.table_type(t)?),
            wasmparser::TypeRef::Memory(m) => crate::EntityType::Memory(reencoder.memory_type(m)),
            wasmparser::TypeRef::Global(g) => crate::EntityType::Global(reencoder.global_type(g)?),
            wasmparser::TypeRef::Tag(t) => crate::EntityType::Tag(reencoder.tag_type(t)),
        })
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the imports to the `import` section.
    pub fn parse_import_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        imports: &mut crate::ImportSection,
        section: wasmparser::ImportSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for import in section {
            reencoder.parse_import(imports, import?)?;
        }
        Ok(())
    }

    /// Parses the single [`wasmparser::Import`] provided and adds it to the
    /// `import` section.
    pub fn parse_import<T: ?Sized + Reencode>(
        reencoder: &mut T,
        imports: &mut crate::ImportSection,
        import: wasmparser::Import<'_>,
    ) -> Result<(), Error<T::Error>> {
        imports.import(
            import.module,
            import.name,
            reencoder.entity_type(import.ty)?,
        );
        Ok(())
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the memories to the `memories` section.
    pub fn parse_memory_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        memories: &mut crate::MemorySection,
        section: wasmparser::MemorySectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for memory in section {
            let memory = memory?;
            memories.memory(reencoder.memory_type(memory));
        }
        Ok(())
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the functions to the `functions` section.
    pub fn parse_function_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        functions: &mut crate::FunctionSection,
        section: wasmparser::FunctionSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for func in section {
            functions.function(reencoder.type_index(func?));
        }
        Ok(())
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the data to the `data` section.
    pub fn parse_data_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        data: &mut crate::DataSection,
        section: wasmparser::DataSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for datum in section {
            reencoder.parse_data(data, datum?)?;
        }
        Ok(())
    }

    /// Parses a single [`wasmparser::Data`] and adds it to the `data` section.
    pub fn parse_data<T: ?Sized + Reencode>(
        reencoder: &mut T,
        data: &mut crate::DataSection,
        datum: wasmparser::Data<'_>,
    ) -> Result<(), Error<T::Error>> {
        match datum.kind {
            wasmparser::DataKind::Active {
                memory_index,
                offset_expr,
            } => data.active(
                reencoder.memory_index(memory_index),
                &reencoder.const_expr(offset_expr)?,
                datum.data.iter().copied(),
            ),
            wasmparser::DataKind::Passive => data.passive(datum.data.iter().copied()),
        };
        Ok(())
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the elements to the `element` section.
    pub fn parse_element_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        elements: &mut crate::ElementSection,
        section: wasmparser::ElementSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for element in section {
            reencoder.parse_element(elements, element?)?;
        }
        Ok(())
    }

    /// Parses the single [`wasmparser::Element`] provided and adds it to the
    /// `element` section.
    pub fn parse_element<T: ?Sized + Reencode>(
        reencoder: &mut T,
        elements: &mut crate::ElementSection,
        element: wasmparser::Element<'_>,
    ) -> Result<(), Error<T::Error>> {
        let mut funcs;
        let mut exprs;
        let elems = match element.items {
            wasmparser::ElementItems::Functions(f) => {
                funcs = Vec::new();
                for func in f {
                    funcs.push(reencoder.function_index(func?));
                }
                crate::Elements::Functions(&funcs)
            }
            wasmparser::ElementItems::Expressions(ty, e) => {
                exprs = Vec::new();
                for expr in e {
                    exprs.push(reencoder.const_expr(expr?)?);
                }
                crate::Elements::Expressions(reencoder.ref_type(ty)?, &exprs)
            }
        };
        match element.kind {
            wasmparser::ElementKind::Active {
                table_index,
                offset_expr,
            } => elements.active(
                table_index.map(|t| reencoder.table_index(t)),
                &reencoder.const_expr(offset_expr)?,
                elems,
            ),
            wasmparser::ElementKind::Passive => elements.passive(elems),
            wasmparser::ElementKind::Declared => elements.declared(elems),
        };
        Ok(())
    }

    pub fn table_index<T: ?Sized + Reencode>(_reencoder: &mut T, table: u32) -> u32 {
        table
    }

    pub fn global_index<T: ?Sized + Reencode>(_reencoder: &mut T, global: u32) -> u32 {
        global
    }

    pub fn data_index<T: ?Sized + Reencode>(_reencoder: &mut T, data: u32) -> u32 {
        data
    }

    pub fn element_index<T: ?Sized + Reencode>(_reencoder: &mut T, element: u32) -> u32 {
        element
    }

    pub fn const_expr<T: ?Sized + Reencode>(
        reencoder: &mut T,
        const_expr: wasmparser::ConstExpr,
    ) -> Result<crate::ConstExpr, Error<T::Error>> {
        let mut ops = const_expr.get_operators_reader().into_iter();

        let result = match ops.next() {
            Some(Ok(wasmparser::Operator::I32Const { value })) => {
                crate::ConstExpr::i32_const(value)
            }
            Some(Ok(wasmparser::Operator::I64Const { value })) => {
                crate::ConstExpr::i64_const(value)
            }
            Some(Ok(wasmparser::Operator::F32Const { value })) => {
                crate::ConstExpr::f32_const(f32::from_bits(value.bits()))
            }
            Some(Ok(wasmparser::Operator::F64Const { value })) => {
                crate::ConstExpr::f64_const(f64::from_bits(value.bits()))
            }
            Some(Ok(wasmparser::Operator::V128Const { value })) => {
                crate::ConstExpr::v128_const(i128::from_le_bytes(*value.bytes()))
            }
            Some(Ok(wasmparser::Operator::RefNull { hty })) => {
                crate::ConstExpr::ref_null(reencoder.heap_type(hty)?)
            }
            Some(Ok(wasmparser::Operator::RefFunc { function_index })) => {
                crate::ConstExpr::ref_func(reencoder.function_index(function_index))
            }
            Some(Ok(wasmparser::Operator::GlobalGet { global_index })) => {
                crate::ConstExpr::global_get(reencoder.global_index(global_index))
            }

            // TODO: support the extended-const proposal.
            Some(Ok(_op)) => return Err(Error::InvalidConstExpr),

            Some(Err(e)) => return Err(Error::ParseError(e)),
            None => return Err(Error::InvalidConstExpr),
        };

        match (ops.next(), ops.next()) {
            (Some(Ok(wasmparser::Operator::End)), None) => Ok(result),
            _ => Err(Error::InvalidConstExpr),
        }
    }

    pub fn block_type<T: ?Sized + Reencode>(
        reencoder: &mut T,
        arg: wasmparser::BlockType,
    ) -> Result<crate::BlockType, Error<T::Error>> {
        match arg {
            wasmparser::BlockType::Empty => Ok(crate::BlockType::Empty),
            wasmparser::BlockType::FuncType(n) => {
                Ok(crate::BlockType::FunctionType(reencoder.type_index(n)))
            }
            wasmparser::BlockType::Type(t) => Ok(crate::BlockType::Result(reencoder.val_type(t)?)),
        }
    }

    pub fn instruction<'a, T: ?Sized + Reencode>(
        reencoder: &mut T,
        arg: wasmparser::Operator<'a>,
    ) -> Result<crate::Instruction<'a>, Error<T::Error>> {
        use crate::Instruction;

        macro_rules! translate {
            ($( @$proposal:ident $op:ident $({ $($arg:ident: $argty:ty),* })? => $visit:ident)*) => {
                Ok(match arg {
                    $(
                        wasmparser::Operator::$op $({ $($arg),* })? => {
                            $(
                                $(let $arg = translate!(map $arg $arg);)*
                            )?
                            translate!(build $op $($($arg)*)?)
                        }
                    )*
                })
            };

            // This case is used to map, based on the name of the field, from the
            // wasmparser payload type to the wasm-encoder payload type through
            // `Translator` as applicable.
            (map $arg:ident tag_index) => (reencoder.tag_index($arg));
            (map $arg:ident function_index) => (reencoder.function_index($arg));
            (map $arg:ident table) => (reencoder.table_index($arg));
            (map $arg:ident table_index) => (reencoder.table_index($arg));
            (map $arg:ident dst_table) => (reencoder.table_index($arg));
            (map $arg:ident src_table) => (reencoder.table_index($arg));
            (map $arg:ident type_index) => (reencoder.type_index($arg));
            (map $arg:ident array_type_index) => (reencoder.type_index($arg));
            (map $arg:ident array_type_index_dst) => (reencoder.type_index($arg));
            (map $arg:ident array_type_index_src) => (reencoder.type_index($arg));
            (map $arg:ident struct_type_index) => (reencoder.type_index($arg));
            (map $arg:ident global_index) => (reencoder.global_index($arg));
            (map $arg:ident mem) => (reencoder.memory_index($arg));
            (map $arg:ident src_mem) => (reencoder.memory_index($arg));
            (map $arg:ident dst_mem) => (reencoder.memory_index($arg));
            (map $arg:ident data_index) => (reencoder.data_index($arg));
            (map $arg:ident elem_index) => (reencoder.element_index($arg));
            (map $arg:ident array_data_index) => (reencoder.data_index($arg));
            (map $arg:ident array_elem_index) => (reencoder.element_index($arg));
            (map $arg:ident blockty) => (reencoder.block_type($arg)?);
            (map $arg:ident relative_depth) => ($arg);
            (map $arg:ident targets) => ((
                $arg
                    .targets()
                    .collect::<Result<Vec<_>, wasmparser::BinaryReaderError>>()?
                    .into(),
                $arg.default(),
            ));
            (map $arg:ident ty) => (reencoder.val_type($arg)?);
            (map $arg:ident hty) => (reencoder.heap_type($arg)?);
            (map $arg:ident from_ref_type) => (reencoder.ref_type($arg)?);
            (map $arg:ident to_ref_type) => (reencoder.ref_type($arg)?);
            (map $arg:ident memarg) => (reencoder.mem_arg($arg));
            (map $arg:ident ordering) => (reencoder.ordering($arg));
            (map $arg:ident local_index) => ($arg);
            (map $arg:ident value) => ($arg);
            (map $arg:ident lane) => ($arg);
            (map $arg:ident lanes) => ($arg);
            (map $arg:ident array_size) => ($arg);
            (map $arg:ident field_index) => ($arg);
            (map $arg:ident try_table) => ($arg);

            // This case takes the arguments of a wasmparser instruction and creates
            // a wasm-encoder instruction. There are a few special cases for where
            // the structure of a wasmparser instruction differs from that of
            // wasm-encoder.
            (build $op:ident) => (Instruction::$op);
            (build BrTable $arg:ident) => (Instruction::BrTable($arg.0, $arg.1));
            (build I32Const $arg:ident) => (Instruction::I32Const($arg));
            (build I64Const $arg:ident) => (Instruction::I64Const($arg));
            (build F32Const $arg:ident) => (Instruction::F32Const(f32::from_bits($arg.bits())));
            (build F64Const $arg:ident) => (Instruction::F64Const(f64::from_bits($arg.bits())));
            (build V128Const $arg:ident) => (Instruction::V128Const($arg.i128()));
            (build TryTable $table:ident) => (Instruction::TryTable(reencoder.block_type($table.ty)?, {
                $table.catches.into_iter().map(|c| reencoder.catch(c)).collect::<Vec<_>>().into()
            }));
            (build $op:ident $arg:ident) => (Instruction::$op($arg));
            (build $op:ident $($arg:ident)*) => (Instruction::$op { $($arg),* });
        }

        wasmparser::for_each_operator!(translate)
    }

    /// Parses the input `section` given from the `wasmparser` crate and adds
    /// all the code to the `code` section.
    pub fn parse_code_section<T: ?Sized + Reencode>(
        reencoder: &mut T,
        code: &mut crate::CodeSection,
        section: wasmparser::CodeSectionReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        for func in section {
            reencoder.parse_function_body(code, func?)?;
        }
        Ok(())
    }

    /// Parses a single [`wasmparser::FunctionBody`] and adds it to the `code` section.
    pub fn parse_function_body<T: ?Sized + Reencode>(
        reencoder: &mut T,
        code: &mut crate::CodeSection,
        func: wasmparser::FunctionBody<'_>,
    ) -> Result<(), Error<T::Error>> {
        let mut f = reencoder.new_function_with_parsed_locals(&func)?;
        let mut reader = func.get_operators_reader()?;
        while !reader.eof() {
            reencoder.parse_instruction(&mut f, &mut reader)?;
        }
        code.function(&f);
        Ok(())
    }

    /// Create a new [`crate::Function`] by parsing the locals declarations from the
    /// provided [`wasmparser::FunctionBody`].
    pub fn new_function_with_parsed_locals<T: ?Sized + Reencode>(
        reencoder: &mut T,
        func: &wasmparser::FunctionBody<'_>,
    ) -> Result<crate::Function, Error<T::Error>> {
        let mut locals = Vec::new();
        for pair in func.get_locals_reader()? {
            let (cnt, ty) = pair?;
            locals.push((cnt, reencoder.val_type(ty)?));
        }
        Ok(crate::Function::new(locals))
    }

    /// Parses a single instruction from `reader` and adds it to `function`.
    pub fn parse_instruction<T: ?Sized + Reencode>(
        reencoder: &mut T,
        function: &mut crate::Function,
        reader: &mut wasmparser::OperatorsReader<'_>,
    ) -> Result<(), Error<T::Error>> {
        function.instruction(&reencoder.instruction(reader.read()?)?);
        Ok(())
    }

    pub fn parse_unknown_section<T: ?Sized + Reencode>(
        _reencoder: &mut T,
        module: &mut crate::Module,
        id: u8,
        contents: &[u8],
    ) -> Result<(), Error<T::Error>> {
        module.section(&crate::RawSection { id, data: contents });
        Ok(())
    }
}

impl From<wasmparser::ComponentValType> for crate::ComponentValType {
    fn from(ty: wasmparser::ComponentValType) -> Self {
        RoundtripReencoder.component_val_type(ty)
    }
}

impl From<wasmparser::TypeBounds> for crate::TypeBounds {
    fn from(ty: wasmparser::TypeBounds) -> Self {
        RoundtripReencoder.type_bounds(ty)
    }
}

impl From<wasmparser::CanonicalOption> for crate::CanonicalOption {
    fn from(opt: wasmparser::CanonicalOption) -> Self {
        RoundtripReencoder.canonical_option(opt)
    }
}

impl From<wasmparser::ComponentExternalKind> for crate::ComponentExportKind {
    fn from(kind: wasmparser::ComponentExternalKind) -> Self {
        RoundtripReencoder.component_export_kind(kind)
    }
}

impl From<wasmparser::ComponentOuterAliasKind> for crate::ComponentOuterAliasKind {
    fn from(kind: wasmparser::ComponentOuterAliasKind) -> Self {
        RoundtripReencoder.component_outer_alias_kind(kind)
    }
}

impl From<wasmparser::ComponentTypeRef> for crate::ComponentTypeRef {
    fn from(ty: wasmparser::ComponentTypeRef) -> Self {
        RoundtripReencoder.component_type_ref(ty)
    }
}

impl From<wasmparser::PrimitiveValType> for crate::PrimitiveValType {
    fn from(ty: wasmparser::PrimitiveValType) -> Self {
        RoundtripReencoder.component_primitive_val_type(ty)
    }
}

impl From<wasmparser::MemArg> for crate::MemArg {
    fn from(arg: wasmparser::MemArg) -> Self {
        RoundtripReencoder.mem_arg(arg)
    }
}

impl From<wasmparser::Ordering> for crate::Ordering {
    fn from(arg: wasmparser::Ordering) -> Self {
        RoundtripReencoder.ordering(arg)
    }
}

impl TryFrom<wasmparser::BlockType> for crate::BlockType {
    type Error = Error;

    fn try_from(arg: wasmparser::BlockType) -> Result<Self, Self::Error> {
        RoundtripReencoder.block_type(arg)
    }
}

impl<'a> TryFrom<wasmparser::Operator<'a>> for crate::Instruction<'a> {
    type Error = Error;

    fn try_from(arg: wasmparser::Operator<'a>) -> Result<Self, Self::Error> {
        RoundtripReencoder.instruction(arg)
    }
}

impl From<wasmparser::Catch> for crate::Catch {
    fn from(arg: wasmparser::Catch) -> Self {
        RoundtripReencoder.catch(arg)
    }
}

impl<'a> TryFrom<wasmparser::ConstExpr<'a>> for crate::ConstExpr {
    type Error = Error;

    fn try_from(const_expr: wasmparser::ConstExpr) -> Result<Self, Self::Error> {
        RoundtripReencoder.const_expr(const_expr)
    }
}

impl<'a> From<wasmparser::CustomSectionReader<'a>> for crate::CustomSection<'a> {
    fn from(section: wasmparser::CustomSectionReader<'a>) -> Self {
        RoundtripReencoder.custom_section(section)
    }
}

impl From<wasmparser::ExternalKind> for crate::ExportKind {
    fn from(external_kind: wasmparser::ExternalKind) -> Self {
        RoundtripReencoder.export_kind(external_kind)
    }
}

impl TryFrom<wasmparser::GlobalType> for crate::GlobalType {
    type Error = Error;

    fn try_from(global_ty: wasmparser::GlobalType) -> Result<Self, Self::Error> {
        RoundtripReencoder.global_type(global_ty)
    }
}

impl TryFrom<wasmparser::TypeRef> for crate::EntityType {
    type Error = Error;

    fn try_from(type_ref: wasmparser::TypeRef) -> Result<Self, Self::Error> {
        RoundtripReencoder.entity_type(type_ref)
    }
}

impl From<wasmparser::MemoryType> for crate::MemoryType {
    fn from(memory_ty: wasmparser::MemoryType) -> Self {
        RoundtripReencoder.memory_type(memory_ty)
    }
}

impl TryFrom<wasmparser::TableType> for crate::TableType {
    type Error = Error;

    fn try_from(table_ty: wasmparser::TableType) -> Result<Self, Self::Error> {
        RoundtripReencoder.table_type(table_ty)
    }
}

impl From<wasmparser::TagKind> for crate::TagKind {
    fn from(kind: wasmparser::TagKind) -> Self {
        RoundtripReencoder.tag_kind(kind)
    }
}

impl From<wasmparser::TagType> for crate::TagType {
    fn from(tag_ty: wasmparser::TagType) -> Self {
        RoundtripReencoder.tag_type(tag_ty)
    }
}

impl TryFrom<wasmparser::SubType> for crate::SubType {
    type Error = Error;

    fn try_from(sub_ty: wasmparser::SubType) -> Result<Self, Self::Error> {
        RoundtripReencoder.sub_type(sub_ty)
    }
}

impl TryFrom<wasmparser::CompositeType> for crate::CompositeType {
    type Error = Error;

    fn try_from(composite_ty: wasmparser::CompositeType) -> Result<Self, Self::Error> {
        RoundtripReencoder.composite_type(composite_ty)
    }
}

impl TryFrom<wasmparser::FuncType> for crate::FuncType {
    type Error = Error;

    fn try_from(func_ty: wasmparser::FuncType) -> Result<Self, Self::Error> {
        RoundtripReencoder.func_type(func_ty)
    }
}

impl TryFrom<wasmparser::ArrayType> for crate::ArrayType {
    type Error = Error;

    fn try_from(array_ty: wasmparser::ArrayType) -> Result<Self, Self::Error> {
        RoundtripReencoder.array_type(array_ty)
    }
}

impl TryFrom<wasmparser::StructType> for crate::StructType {
    type Error = Error;

    fn try_from(struct_ty: wasmparser::StructType) -> Result<Self, Self::Error> {
        RoundtripReencoder.struct_type(struct_ty)
    }
}

impl TryFrom<wasmparser::FieldType> for crate::FieldType {
    type Error = Error;

    fn try_from(field_ty: wasmparser::FieldType) -> Result<Self, Self::Error> {
        RoundtripReencoder.field_type(field_ty)
    }
}

impl TryFrom<wasmparser::StorageType> for crate::StorageType {
    type Error = Error;

    fn try_from(storage_ty: wasmparser::StorageType) -> Result<Self, Self::Error> {
        RoundtripReencoder.storage_type(storage_ty)
    }
}

impl TryFrom<wasmparser::ValType> for crate::ValType {
    type Error = Error;

    fn try_from(val_ty: wasmparser::ValType) -> Result<Self, Self::Error> {
        RoundtripReencoder.val_type(val_ty)
    }
}

impl TryFrom<wasmparser::RefType> for crate::RefType {
    type Error = Error;

    fn try_from(ref_type: wasmparser::RefType) -> Result<Self, Self::Error> {
        RoundtripReencoder.ref_type(ref_type)
    }
}

impl TryFrom<wasmparser::HeapType> for crate::HeapType {
    type Error = Error;

    fn try_from(heap_type: wasmparser::HeapType) -> Result<Self, Self::Error> {
        crate::reencode::utils::heap_type(&mut crate::reencode::RoundtripReencoder, heap_type)
    }
}

impl From<wasmparser::AbstractHeapType> for crate::AbstractHeapType {
    fn from(value: wasmparser::AbstractHeapType) -> Self {
        RoundtripReencoder.abstract_heap_type(value)
    }
}
