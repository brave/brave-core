use crate::map::IdHashMap;
use crate::{DataId, ElementId, Function, FunctionId, GlobalId, Result};
use crate::{LocalId, MemoryId, TableId, TypeId};
use anyhow::bail;

/// Maps from old indices in the original Wasm binary to `walrus` IDs.
///
/// This is intended to be used with `walrus::Module`s that were parsed from
/// some existing Wasm binary. `walrus::Module`s that are built up from scratch,
/// and not originally parsed from an existing Wasm binary, will have an empty
/// `IndicesToIds`.
///
/// For example, this allows you to get the `walrus::FunctionId` of some Wasm
/// function when you have its old index in the original Wasm module.
///
/// Any newly built or added things (functions, tables, types, etc) are not
/// associated with an old index (since they were not present in the original
/// Wasm binary).
#[derive(Debug, Default)]
pub struct IndicesToIds {
    tables: Vec<TableId>,
    types: Vec<TypeId>,
    funcs: Vec<FunctionId>,
    globals: Vec<GlobalId>,
    memories: Vec<MemoryId>,
    elements: Vec<ElementId>,
    data: Vec<DataId>,
    locals: IdHashMap<Function, Vec<LocalId>>,
}

macro_rules! define_push_get {
    ( $push:ident, $get:ident, $id_ty:ty, $member:ident ) => {
        impl IndicesToIds {
            /// Pushes a new local ID to map it to the next index internally
            pub(crate) fn $push(&mut self, id: $id_ty) -> u32 {
                self.$member.push(id);
                (self.$member.len() - 1) as u32
            }

            /// Gets the ID for a particular index.
            ///
            /// If the index did not exist in the original Wasm binary, an `Err`
            /// is returned.
            pub fn $get(&self, index: u32) -> Result<$id_ty> {
                match self.$member.get(index as usize) {
                    Some(x) => Ok(*x),
                    None => bail!(
                        "index `{}` is out of bounds for {}",
                        index,
                        stringify!($member)
                    ),
                }
            }
        }
    };
}

define_push_get!(push_table, get_table, TableId, tables);
define_push_get!(push_type, get_type, TypeId, types);
define_push_get!(push_func, get_func, FunctionId, funcs);
define_push_get!(push_global, get_global, GlobalId, globals);
define_push_get!(push_memory, get_memory, MemoryId, memories);
define_push_get!(push_element, get_element, ElementId, elements);
define_push_get!(push_data, get_data, DataId, data);

impl IndicesToIds {
    /// Pushes a new local ID to map it to the next index internally
    pub(crate) fn push_local(&mut self, function: FunctionId, id: LocalId) -> u32 {
        let list = self.locals.entry(function).or_default();
        list.push(id);
        (list.len() as u32) - 1
    }

    /// Gets the ID for a particular index
    pub fn get_local(&self, function: FunctionId, index: u32) -> Result<LocalId> {
        let locals = match self.locals.get(&function) {
            Some(x) => x,
            None => bail!(
                "function index `{}` is out of bounds for local",
                function.index()
            ),
        };
        match locals.get(index as usize) {
            Some(x) => Ok(*x),
            None => bail!(
                "index `{}` in function `{}` is out of bounds for local",
                index,
                function.index(),
            ),
        }
    }
}
