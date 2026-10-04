//! Traits and code for emitting high-level structures as low-level, raw wasm
//! structures. E.g. translating from globally unique identifiers down to the
//! raw wasm structure's index spaces.

use crate::ir::Local;
use crate::map::{IdHashMap, IdHashSet};
use crate::{CodeTransform, Global, GlobalId, Memory, MemoryId, Module, Table, TableId};
use crate::{Data, DataId, Element, ElementId, Function, FunctionId};
use crate::{Type, TypeId};

pub struct EmitContext<'a> {
    pub module: &'a Module,
    pub indices: &'a mut IdsToIndices,
    pub wasm_module: wasm_encoder::Module,
    pub locals: IdHashMap<Function, IdHashSet<Local>>,
    pub code_transform: CodeTransform,
}

/// Anything that can be lowered to raw wasm structures.
pub trait Emit {
    /// Emit `self` into the given context.
    fn emit(&self, cx: &mut EmitContext);
}

impl<'a, T: ?Sized + Emit> Emit for &'a T {
    fn emit(&self, cx: &mut EmitContext) {
        T::emit(self, cx)
    }
}

/// Maps our high-level identifiers to the raw indices they end up emitted at.
///
/// As we lower to raw wasm structures, we cement various constructs' locations
/// in their respective index spaces. For example, a type with some id `A` ends
/// up being the `i^th` type emitted in the raw wasm type section. When a
/// function references that type, it needs to reference it by its `i` index
/// since the identifier `A` doesn't exist at the raw wasm level.
#[derive(Debug, Default)]
pub struct IdsToIndices {
    tables: IdHashMap<Table, u32>,
    types: IdHashMap<Type, u32>,
    funcs: IdHashMap<Function, u32>,
    globals: IdHashMap<Global, u32>,
    memories: IdHashMap<Memory, u32>,
    elements: IdHashMap<Element, u32>,
    data: IdHashMap<Data, u32>,
    pub(crate) locals: IdHashMap<Function, IdHashMap<Local, u32>>,
}

macro_rules! define_get_index {
    ( $(
        $get_name:ident, $id_ty:ty, $member:ident;
    )* ) => {
        impl IdsToIndices {
            $(
                /// Get the index for the given identifier.
                #[inline]
                pub fn $get_name(&self, id: $id_ty) -> u32 {
                    self.$member.get(&id).cloned().unwrap_or_else(|| panic!(
                        "{}: Should never try and get the index for an identifier that has not already had \
                         its index set. This means that either we are attempting to get the index of \
                         an unused identifier, or that we are emitting sections in the wrong order. \n\n\
                         id = {:?}",
                        stringify!($get_name),
                        id,
                    ))
                }
            )*
        }
    };
}

macro_rules! define_get_push_index {
    ( $(
        $get_name:ident, $push_name:ident, $id_ty:ty, $member:ident;
    )* ) => {
        define_get_index!( $( $get_name, $id_ty, $member; )* );
        impl IdsToIndices {
            $(
                /// Adds the given identifier to this set, assigning it the next
                /// available index.
                #[inline]
                pub(crate) fn $push_name(&mut self, id: $id_ty) {
                    let idx = self.$member.len() as u32;
                    log::trace!(concat!(stringify!($push_name),": assigning index {} to {:?}"), idx, id);
                    self.$member.insert(id, idx);
                }
            )*
        }
    };
}

define_get_push_index! {
    get_table_index, push_table, TableId, tables;
    get_type_index, push_type, TypeId, types;
    get_func_index, push_func, FunctionId, funcs;
    get_global_index, push_global, GlobalId, globals;
    get_memory_index, push_memory, MemoryId, memories;
    get_element_index, push_element, ElementId, elements;
}
define_get_index! {
    get_data_index, DataId, data;
}

impl IdsToIndices {
    /// Sets the data index to the specified value
    pub(crate) fn set_data_index(&mut self, id: DataId, idx: u32) {
        self.data.insert(id, idx);
    }
}
