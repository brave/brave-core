use gimli::write::{DebuggingInformationEntry, Unit, UnitEntryId};

pub(crate) struct DebuggingInformationCursor<'a> {
    entry_id_stack: Vec<UnitEntryId>,

    unit: &'a mut Unit,

    called_next_dfs: bool,
}

impl<'a> DebuggingInformationCursor<'a> {
    pub fn new(unit: &'a mut Unit) -> Self {
        Self {
            unit,
            entry_id_stack: Vec::new(),
            called_next_dfs: false,
        }
    }

    pub fn current(&mut self) -> Option<&mut DebuggingInformationEntry> {
        if !self.entry_id_stack.is_empty() {
            Some(self.unit.get_mut(*self.entry_id_stack.last().unwrap()))
        } else {
            None
        }
    }

    pub fn next_dfs(&mut self) -> Option<&mut DebuggingInformationEntry> {
        if !self.called_next_dfs {
            let root = self.unit.root();
            self.entry_id_stack.push(root);
            self.called_next_dfs = true;
            return self.current();
        }

        if self.entry_id_stack.is_empty() {
            return None;
        }

        let last_element_id = self.entry_id_stack.pop().unwrap();
        let last_element = self.unit.get_mut(last_element_id);

        self.entry_id_stack.append(
            &mut last_element
                .children()
                .map(UnitEntryId::clone)
                .rev()
                .collect(),
        );

        self.current()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use gimli::constants;
    use gimli::write::*;
    use gimli::{Encoding, Format};

    #[test]
    fn test_create_instance() {
        let mut unit1 = Unit::new(
            Encoding {
                version: 4,
                address_size: 8,
                format: Format::Dwarf32,
            },
            LineProgram::none(),
        );

        let root_id = unit1.root();
        let child1_id = unit1.add(root_id, constants::DW_TAG_subprogram);
        let child2_id = unit1.add(child1_id, constants::DW_TAG_lexical_block);

        let mut cursor = DebuggingInformationCursor::new(&mut unit1);

        assert!(cursor.current().is_none());
        assert_eq!(cursor.next_dfs().unwrap().id(), root_id);
        assert_eq!(cursor.next_dfs().unwrap().id(), child1_id);
        assert_eq!(cursor.next_dfs().unwrap().id(), child2_id);
        assert!(cursor.next_dfs().is_none());
    }
}
