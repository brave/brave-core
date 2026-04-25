//! Removes any non-referenced items from a module
//!
//! This commit will remove functions, data, etc, that are not referenced
//! internally and can be safely removed.

use crate::map::IdHashSet;
use crate::passes::used::Used;
use crate::{ImportKind, Module};
use id_arena::Id;

/// Run GC passes over the module specified.
pub fn run(m: &mut Module) {
    let used = Used::new(m);

    let mut unused_imports = Vec::new();
    for import in m.imports.iter() {
        let used = match &import.kind {
            ImportKind::Function(f) => used.funcs.contains(f),
            ImportKind::Table(t) => used.tables.contains(t),
            ImportKind::Global(g) => used.globals.contains(g),
            ImportKind::Memory(m) => used.memories.contains(m),
        };
        if !used {
            unused_imports.push(import.id());
        }
    }
    for id in unused_imports {
        m.imports.delete(id);
    }

    for id in unused(&used.tables, m.tables.iter().map(|t| t.id())) {
        m.tables.delete(id);
    }
    for id in unused(&used.globals, m.globals.iter().map(|t| t.id())) {
        m.globals.delete(id);
    }
    for id in unused(&used.memories, m.memories.iter().map(|t| t.id())) {
        m.memories.delete(id);
    }
    for id in unused(&used.data, m.data.iter().map(|t| t.id())) {
        m.data.delete(id);
    }
    for id in unused(&used.elements, m.elements.iter().map(|t| t.id())) {
        m.elements.delete(id);
    }
    for id in unused(&used.types, m.types.iter().map(|t| t.id())) {
        m.types.delete(id);
    }
    for id in unused(&used.funcs, m.funcs.iter().map(|t| t.id())) {
        m.funcs.delete(id);
    }
}

fn unused<T>(used: &IdHashSet<T>, all: impl Iterator<Item = Id<T>>) -> Vec<Id<T>> {
    let mut unused = Vec::new();
    for id in all {
        if !used.contains(&id) {
            unused.push(id);
        }
    }
    unused
}
