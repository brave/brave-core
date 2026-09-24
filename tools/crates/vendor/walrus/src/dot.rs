//! Utilities for emitting GraphViz dot files.

use crate::ir::*;
use crate::*;
use std::fs;
use std::path::Path;

impl Module {
    /// Generate a [GraphViz Dot](https://graphviz.org/) file for this module,
    /// showing the relationship between various structures in the module and
    /// its instructions.
    ///
    /// # Example
    ///
    /// First, generate a `.dot` file with this method:
    ///
    /// ```
    /// # fn foo() -> walrus::Result<()> {
    /// # let get_module_from_somewhere = || unimplemented!();
    /// let my_module: walrus::Module = get_module_from_somewhere();
    /// my_module.write_graphviz_dot("my_module.dot")?;
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// Second, use the `dot` command-line tool to render an SVG (or PNG,
    /// etc...):
    ///
    /// ```bash
    /// dot my_module.dot \       # Provide our generated `.dot`.
    ///     -T svg \              # Generate an SVG image.
    ///     -o my_module.svg      # Write to this output file.
    /// ```
    pub fn write_graphviz_dot(&self, path: impl AsRef<Path>) -> Result<()> {
        let mut dot_string = String::new();
        self.dot(&mut dot_string);
        fs::write(path, dot_string)?;
        Ok(())
    }
}

trait Dot {
    /// Append a top-level graphviz dot form to the `out` string.
    fn dot(&self, out: &mut String);
}

trait DotName {
    /// Get this thing's unique name in the graphviz dot file.
    fn dot_name(&self) -> String;
}

trait FieldAggregator {
    /// Add a field for the current node.
    fn add_field(&mut self, field: &[&str]);

    /// Add a field with a named port for the current node.
    fn add_field_with_port(&mut self, port: &str, field: &str);
}

trait EdgeAggregator {
    /// Add an outgoing edge from the current node.
    fn add_edge(&mut self, to: &impl DotName);

    /// Add an outgoing edge form the current node at a specific port.
    fn add_edge_from_port(&mut self, port: &str, to: &impl DotName);
}

/// A trait for generating a top-level node with multiple fields and some number
/// of edges to other nodes.
///
/// Anything that implements this trait automatically gets nice HTML tables for
/// the fields and doesn't have to worry about the details of serializing to the
/// Dot language, just have to add fields/edges on the respective aggregator.
trait DotNode: DotName {
    /// For each field that should show up in this node's record, call
    /// `fields.add_field([...])`.
    fn fields(&self, fields: &mut impl FieldAggregator);

    /// For each outgoing edge from this node to another node, call
    /// `edges.add_edge(..)`.
    fn edges(&self, edges: &mut impl EdgeAggregator);
}

impl<T: DotNode> Dot for T {
    fn dot(&self, out: &mut String) {
        let dot_name = self.dot_name();

        out.push_str("    ");
        out.push_str(&dot_name);
        out.push_str(" [shape=\"none\", label=<<table align=\"left\" cellborder=\"0\">");
        self.fields(&mut AppendFields { out });
        out.push_str("</table>>];\n");

        self.edges(&mut AppendEdges {
            out,
            from: &dot_name,
        });
        return;

        struct AppendFields<'a> {
            out: &'a mut String,
        }

        impl FieldAggregator for AppendFields<'_> {
            fn add_field(&mut self, field: &[&str]) {
                assert!(!field.is_empty());
                self.out.push_str("<tr>");
                for f in field {
                    self.out.push_str("<td>");
                    self.out.push_str(f);
                    self.out.push_str("</td>");
                }
                self.out.push_str("</tr>");
            }

            fn add_field_with_port(&mut self, port: &str, field: &str) {
                assert!(!field.is_empty());
                self.out.push_str("<tr>");
                self.out.push_str("<td port=\"");
                self.out.push_str(port);
                self.out.push_str("\">");
                self.out.push_str(field);
                self.out.push_str("</td>");
                self.out.push_str("</tr>");
            }
        }

        struct AppendEdges<'a> {
            out: &'a mut String,
            from: &'a str,
        }

        impl EdgeAggregator for AppendEdges<'_> {
            fn add_edge(&mut self, to: &impl DotName) {
                self.out.push_str("    ");
                self.out.push_str(self.from);
                self.out.push_str(" -> ");
                self.out.push_str(&to.dot_name());
                self.out.push_str(";\n");
            }

            fn add_edge_from_port(&mut self, port: &str, to: &impl DotName) {
                self.out.push_str("    ");
                self.out.push_str(self.from);
                self.out.push(':');
                self.out.push_str(port);
                self.out.push_str(" -> ");
                self.out.push_str(&to.dot_name());
                self.out.push_str(";\n");
            }
        }
    }
}

impl Dot for Module {
    fn dot(&self, out: &mut String) {
        out.push_str("digraph {\n");

        self.imports.dot(out);
        self.tables.dot(out);
        self.types.dot(out);
        self.funcs.dot(out);
        self.globals.dot(out);
        self.locals.dot(out);
        self.exports.dot(out);
        self.memories.dot(out);
        self.data.dot(out);
        self.elements.dot(out);

        // TODO?
        // self.start.dot(out);
        // self.producers.dot(out);
        // self.customs.dot(out);
        // self.name.dot(out);
        // self.config.dot(out);

        out.push('}');
    }
}

macro_rules! impl_dot_name_for_id {
    ( $( $id:ident; )* ) => {
        $(
            impl DotName for $id {
                fn dot_name(&self) -> String {
                    // NB: the hash contains the arena id as well as the index,
                    // which is important for differentiating instruction
                    // sequences that have the same arena index but live in
                    // different function's arenas.
                    use std::hash::{Hash, Hasher};
                    let mut hasher = crate::map::IdHasher::default();
                    self.hash(&mut hasher);
                    format!("{}_{:x}", stringify!($id), hasher.finish())
                }
            }
        )*
    }
}

impl_dot_name_for_id! {
    ImportId;
    TableId;
    TypeId;
    FunctionId;
    GlobalId;
    LocalId;
    ExportId;
    MemoryId;
    DataId;
    ElementId;
    InstrSeqId;
}

macro_rules! impl_dot_via_iter {
    ( $( $t:ty; )* ) => {
        $(
            impl Dot for $t {
                fn dot(&self, out: &mut String) {
                    out.push_str(concat!("    // ", stringify!($t), "\n"));
                    for x in self.iter() {
                        x.dot(out);
                    }
                    out.push_str("\n");
                }
            }
        )*
    }
}

impl_dot_via_iter! {
    ModuleImports;
    ModuleTables;
    ModuleTypes;
    ModuleFunctions;
    ModuleGlobals;
    ModuleLocals;
    ModuleExports;
    ModuleMemories;
    ModuleData;
    ModuleElements;
}

macro_rules! impl_dot_name_via_id {
    ( $( $t:ty; )* ) => {
        $(
            impl DotName for $t {
                fn dot_name(&self) -> String {
                    self.id().dot_name()
                }
            }
        )*
    }
}

impl_dot_name_via_id! {
    Import;
    Table;
    Type;
    Function;
    Global;
    Local;
    Export;
    Memory;
    Data;
    Element;
    InstrSeq;
}

impl DotNode for Import {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Import {:?}</b>", self.id())]);
        fields.add_field(&["module", &self.module]);
        fields.add_field(&["name", &self.name]);
    }

    fn edges(&self, _edges: &mut impl EdgeAggregator) {}
}

impl DotNode for Table {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Table {:?}</b>", self.id())]);
        fields.add_field(&["initial", &self.initial.to_string()]);
        fields.add_field(&["maximum", &format!("{:?}", self.maximum)]);
        if self.import.is_some() {
            fields.add_field_with_port("import", "import");
        }
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        if let Some(imp) = self.import {
            edges.add_edge_from_port("import", &imp);
        }
    }
}

impl DotNode for Type {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Type {:?}</b>", self.id())]);
        fields.add_field(&["params", &format!("{:?}", self.params())]);
        fields.add_field(&["results", &format!("{:?}", self.results())]);
    }

    fn edges(&self, _edges: &mut impl EdgeAggregator) {}
}

impl Dot for Function {
    fn dot(&self, out: &mut String) {
        FunctionHeader(self).dot(out);
        if let FunctionKind::Local(ref l) = self.kind {
            l.dot(out);
        }
    }
}

struct FunctionHeader<'a>(&'a Function);

impl std::ops::Deref for FunctionHeader<'_> {
    type Target = Function;
    fn deref(&self) -> &Function {
        self.0
    }
}

impl DotName for FunctionHeader<'_> {
    fn dot_name(&self) -> String {
        self.0.dot_name()
    }
}

impl DotNode for FunctionHeader<'_> {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Function {:?}</b>", self.id())]);
        if let Some(name) = self.name.as_ref() {
            fields.add_field(&["name", name]);
        }
        fields.add_field_with_port("type", "type");
        match &self.kind {
            FunctionKind::Import(_) => {
                fields.add_field_with_port("import", "import");
            }
            FunctionKind::Local(_) => {
                fields.add_field_with_port("body", "body");
            }
            FunctionKind::Uninitialized(_) => unreachable!(),
        }
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        edges.add_edge_from_port("type", &self.ty());
        match &self.kind {
            FunctionKind::Import(imp_func) => {
                edges.add_edge_from_port("import", &imp_func.import);
            }
            FunctionKind::Local(local_func) => {
                edges.add_edge_from_port("body", local_func);
            }
            FunctionKind::Uninitialized(_) => unreachable!(),
        }
    }
}

impl DotName for LocalFunction {
    fn dot_name(&self) -> String {
        self.entry_block().dot_name()
    }
}

impl Dot for LocalFunction {
    fn dot(&self, out: &mut String) {
        let visitor = &mut DotVisitor { out };
        dfs_in_order(visitor, self, self.entry_block());

        struct DotVisitor<'a> {
            out: &'a mut String,
        }

        impl<'a, 'instr> Visitor<'instr> for DotVisitor<'a> {
            fn start_instr_seq(&mut self, seq: &'instr InstrSeq) {
                seq.dot(self.out);
            }
        }
    }
}

impl DotNode for InstrSeq {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        for (i, (instr, _)) in self.instrs.iter().enumerate() {
            fields.add_field_with_port(&i.to_string(), &format!("{:?}", instr));
        }
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        for (i, (instr, _)) in self.instrs.iter().enumerate() {
            let port = i.to_string();
            instr.visit(&mut DotVisitor { port, edges });
        }

        struct DotVisitor<'a, E> {
            port: String,
            edges: &'a mut E,
        }

        impl<'a, 'instr, E> Visitor<'instr> for DotVisitor<'a, E>
        where
            E: EdgeAggregator,
        {
            fn visit_instr_seq_id(&mut self, instr_seq_id: &InstrSeqId) {
                self.edges.add_edge_from_port(&self.port, instr_seq_id);
            }

            fn visit_local_id(&mut self, local: &crate::LocalId) {
                self.edges.add_edge_from_port(&self.port, local);
            }

            fn visit_memory_id(&mut self, memory: &crate::MemoryId) {
                self.edges.add_edge_from_port(&self.port, memory);
            }

            fn visit_table_id(&mut self, table: &crate::TableId) {
                self.edges.add_edge_from_port(&self.port, table);
            }

            fn visit_global_id(&mut self, global: &crate::GlobalId) {
                self.edges.add_edge_from_port(&self.port, global);
            }

            fn visit_function_id(&mut self, function: &crate::FunctionId) {
                self.edges.add_edge_from_port(&self.port, function);
            }

            fn visit_data_id(&mut self, data: &crate::DataId) {
                self.edges.add_edge_from_port(&self.port, data);
            }

            fn visit_type_id(&mut self, ty: &crate::TypeId) {
                self.edges.add_edge_from_port(&self.port, ty);
            }
        }
    }
}

impl DotNode for Global {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Global {:?}</b>", self.id())]);
        fields.add_field_with_port("type", "type");
        fields.add_field(&["mutable", if self.mutable { "true" } else { "false" }]);
        match self.kind {
            GlobalKind::Import(_imp) => {
                fields.add_field_with_port("import", "import");
            }
            GlobalKind::Local(_init) => {
                // TODO FITZGEN
            }
        }
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        if let GlobalKind::Import(imp) = self.kind {
            edges.add_edge_from_port("import", &imp);
        }
    }
}

impl DotNode for Local {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Local {:?}</b>", self.id())]);
        fields.add_field(&["type", &format!("{:?}", self.ty())]);
    }

    fn edges(&self, _edges: &mut impl EdgeAggregator) {}
}

impl DotNode for Export {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Export {:?}</b>", self.id())]);
        fields.add_field(&["name", &self.name]);
        fields.add_field_with_port("item", "item");
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        match self.item {
            ExportItem::Function(f) => edges.add_edge_from_port("item", &f),
            ExportItem::Table(t) => edges.add_edge_from_port("item", &t),
            ExportItem::Memory(m) => edges.add_edge_from_port("item", &m),
            ExportItem::Global(g) => edges.add_edge_from_port("item", &g),
        }
    }
}

impl DotNode for Memory {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Memory {:?}</b>", self.id())]);
        fields.add_field(&["shared", if self.shared { "true" } else { "false" }]);
        fields.add_field(&["initial", &self.initial.to_string()]);
        fields.add_field(&["maximum", &format!("{:?}", self.maximum)]);
        if self.import.is_some() {
            fields.add_field_with_port("import", "import");
        }
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        if let Some(imp) = self.import {
            edges.add_edge_from_port("import", &imp);
        }
    }
}

impl DotNode for Data {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Data {:?}</b>", self.id())]);
        fields.add_field_with_port("kind", &format!("{:?}", self.kind));
        // `self.value` ommitted because it is likely too big and just gibberish
        // anyways.
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        if let DataKind::Active { memory, offset: _ } = self.kind {
            edges.add_edge_from_port("kind", &memory);
        }
    }
}

impl DotNode for Element {
    fn fields(&self, fields: &mut impl FieldAggregator) {
        fields.add_field(&[&format!("<b>Element {:?}</b>", self.id())]);
    }

    fn edges(&self, edges: &mut impl EdgeAggregator) {
        if let ElementItems::Functions(function_ids) = &self.items {
            function_ids.iter().for_each(|f| edges.add_edge(f));
        }
    }
}
