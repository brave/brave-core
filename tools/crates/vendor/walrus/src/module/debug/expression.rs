use crate::{CodeTransform, Function, InstrLocId, ModuleFunctions};
use id_arena::Id;
use std::{cmp::Ordering, ops::Range};

use super::dwarf::AddressSearchPreference;

/// Specify roles of origial address.
#[derive(Debug, PartialEq)]
pub(crate) enum CodeAddress {
    /// The address is instruction within a function.
    InstrInFunction { instr_id: InstrLocId },
    /// The address is one byte before the instruction.
    InstrEdge { instr_id: InstrLocId },
    /// The address is within a function, but does not match any instruction.
    OffsetInFunction { id: Id<Function>, offset: usize },
    /// The address is boundary of functions. Equals to OffsetInFunction with offset(section size).
    FunctionEdge { id: Id<Function> },
    /// The address is unknown.
    Unknown,
}

/// Converts original code address to CodeAddress
pub(crate) struct CodeAddressGenerator {
    /// Function range based convert table
    address_convert_table: Vec<(Range<usize>, Id<Function>)>,
    /// Instrument based convert table
    instrument_address_convert_table: Vec<(usize, InstrLocId)>,
}

impl CodeAddressGenerator {
    pub(crate) fn new(funcs: &ModuleFunctions) -> Self {
        let mut address_convert_table = funcs
            .iter_local()
            .filter_map(|(func_id, func)| func.original_range.clone().map(|range| (range, func_id)))
            .collect::<Vec<_>>();

        let mut instrument_address_convert_table = funcs
            .iter_local()
            .flat_map(|(_, func)| &func.instruction_mapping)
            .copied()
            .collect::<Vec<_>>();

        address_convert_table.sort_by_key(|i| i.0.start);
        instrument_address_convert_table.sort_by_key(|i| i.0);

        Self {
            address_convert_table,
            instrument_address_convert_table,
        }
    }

    pub(crate) fn find_address(
        &self,
        address: usize,
        search_preference: AddressSearchPreference,
    ) -> CodeAddress {
        match self
            .instrument_address_convert_table
            .binary_search_by_key(&address, |i| i.0)
        {
            Ok(id) => {
                return CodeAddress::InstrInFunction {
                    instr_id: self.instrument_address_convert_table[id].1,
                }
            }
            Err(id) => {
                if id < self.instrument_address_convert_table.len()
                    && self.instrument_address_convert_table[id].0 - 1 == address
                {
                    return CodeAddress::InstrEdge {
                        instr_id: self.instrument_address_convert_table[id].1,
                    };
                }
            }
        };

        // If the address is not mapped to any instruction, falling back to function-range-based comparison.
        let inclusive_range_comparor = |range: &(Range<usize>, Id<Function>)| {
            // range.start < address <= range.end
            if range.0.end < address {
                Ordering::Less
            } else if address <= range.0.start {
                Ordering::Greater
            } else {
                Ordering::Equal
            }
        };
        let exclusive_range_comparor = |range: &(Range<usize>, Id<Function>)| {
            // normal comparison: range.start <= address < range.end
            if range.0.end <= address {
                Ordering::Less
            } else if address < range.0.start {
                Ordering::Greater
            } else {
                Ordering::Equal
            }
        };
        let range_comparor: &dyn Fn(_) -> Ordering = match search_preference {
            AddressSearchPreference::InclusiveFunctionEnd => &inclusive_range_comparor,
            AddressSearchPreference::ExclusiveFunctionEnd => &exclusive_range_comparor,
        };

        match self.address_convert_table.binary_search_by(range_comparor) {
            Ok(i) => {
                let entry = &self.address_convert_table[i];
                let code_offset_from_function_start = address - entry.0.start;

                if address == entry.0.end {
                    CodeAddress::FunctionEdge { id: entry.1 }
                } else {
                    CodeAddress::OffsetInFunction {
                        id: entry.1,
                        offset: code_offset_from_function_start,
                    }
                }
            }
            Err(_) => CodeAddress::Unknown,
        }
    }
}

/// Converts CodeAddress to translated code address
pub(crate) struct CodeAddressConverter<'a> {
    code_transform: &'a CodeTransform,
}

impl<'a> CodeAddressConverter<'a> {
    pub(crate) fn new(code_transform: &'a CodeTransform) -> Self {
        Self { code_transform }
    }

    pub(crate) fn find_address(&self, code: CodeAddress) -> Option<usize> {
        match code {
            CodeAddress::InstrInFunction { instr_id } => {
                match self
                    .code_transform
                    .instruction_map
                    .binary_search_by_key(&instr_id, |i| i.0)
                {
                    Ok(id) => Some(self.code_transform.instruction_map[id].1),
                    Err(_) => None,
                }
            }
            CodeAddress::InstrEdge { instr_id } => {
                match self
                    .code_transform
                    .instruction_map
                    .binary_search_by_key(&instr_id, |i| i.0)
                {
                    Ok(id) => Some(self.code_transform.instruction_map[id].1 - 1),
                    Err(_) => None,
                }
            }
            CodeAddress::OffsetInFunction { id, offset } => {
                match self
                    .code_transform
                    .function_ranges
                    .binary_search_by_key(&id, |i| i.0)
                {
                    Ok(id) => Some(self.code_transform.function_ranges[id].1.start + offset),
                    Err(_) => None,
                }
            }
            CodeAddress::FunctionEdge { id } => {
                match self
                    .code_transform
                    .function_ranges
                    .binary_search_by_key(&id, |i| i.0)
                {
                    Ok(id) => Some(self.code_transform.function_ranges[id].1.end),
                    Err(_) => None,
                }
            }
            CodeAddress::Unknown => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_code_address_generator() {
        let mut module = crate::Module::default();

        let mut func1 = crate::LocalFunction::new(
            Vec::new(),
            crate::FunctionBuilder::new(&mut module.types, &[], &[]),
        );

        func1.original_range = Some(Range { start: 20, end: 30 });

        let id1 = module.funcs.add_local(func1);

        let mut func2 = crate::LocalFunction::new(
            Vec::new(),
            crate::FunctionBuilder::new(&mut module.types, &[], &[]),
        );

        func2.original_range = Some(Range { start: 30, end: 50 });

        let id2 = module.funcs.add_local(func2);

        let address_converter = CodeAddressGenerator::new(&module.funcs);

        assert_eq!(
            address_converter.find_address(10, AddressSearchPreference::InclusiveFunctionEnd),
            CodeAddress::Unknown
        );
        assert_eq!(
            address_converter.find_address(20, AddressSearchPreference::ExclusiveFunctionEnd),
            CodeAddress::OffsetInFunction { id: id1, offset: 0 }
        );
        assert_eq!(
            address_converter.find_address(20, AddressSearchPreference::InclusiveFunctionEnd),
            CodeAddress::Unknown
        );
        assert_eq!(
            address_converter.find_address(25, AddressSearchPreference::ExclusiveFunctionEnd),
            CodeAddress::OffsetInFunction { id: id1, offset: 5 }
        );
        assert_eq!(
            address_converter.find_address(29, AddressSearchPreference::ExclusiveFunctionEnd),
            CodeAddress::OffsetInFunction { id: id1, offset: 9 }
        );
        assert_eq!(
            address_converter.find_address(29, AddressSearchPreference::InclusiveFunctionEnd),
            CodeAddress::OffsetInFunction { id: id1, offset: 9 }
        );
        assert_eq!(
            address_converter.find_address(30, AddressSearchPreference::InclusiveFunctionEnd),
            CodeAddress::FunctionEdge { id: id1 }
        );
        assert_eq!(
            address_converter.find_address(30, AddressSearchPreference::ExclusiveFunctionEnd),
            CodeAddress::OffsetInFunction { id: id2, offset: 0 }
        );
        assert_eq!(
            address_converter.find_address(50, AddressSearchPreference::InclusiveFunctionEnd),
            CodeAddress::FunctionEdge { id: id2 }
        );
        assert_eq!(
            address_converter.find_address(50, AddressSearchPreference::ExclusiveFunctionEnd),
            CodeAddress::Unknown
        );
    }

    #[test]
    fn test_code_address_converter() {
        let mut module = crate::Module::default();

        let func1 = crate::LocalFunction::new(
            Vec::new(),
            crate::FunctionBuilder::new(&mut module.types, &[], &[]),
        );
        let id1 = module.funcs.add_local(func1);
        let instr_id1 = InstrLocId::new(1);
        let instr_id2 = InstrLocId::new(2);

        let mut code_transform = CodeTransform::default();
        {
            code_transform
                .function_ranges
                .push((id1, Range { start: 50, end: 80 }));
            code_transform.instruction_map.push((instr_id1, 60));
            code_transform.instruction_map.push((instr_id2, 65));
        }

        let converter = CodeAddressConverter::new(&code_transform);

        assert_eq!(
            converter.find_address(CodeAddress::OffsetInFunction { id: id1, offset: 5 }),
            Some(55)
        );
        assert_eq!(
            converter.find_address(CodeAddress::InstrInFunction {
                instr_id: instr_id1
            }),
            Some(60)
        );
        assert_eq!(
            converter.find_address(CodeAddress::InstrEdge {
                instr_id: instr_id2
            }),
            Some(64)
        );
        assert_eq!(
            converter.find_address(CodeAddress::InstrInFunction {
                instr_id: instr_id2
            }),
            Some(65)
        );
        assert_eq!(
            converter.find_address(CodeAddress::FunctionEdge { id: id1 }),
            Some(80)
        );
        assert_eq!(converter.find_address(CodeAddress::Unknown), None);
    }
}
