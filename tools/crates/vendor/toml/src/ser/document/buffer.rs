use alloc::collections::BTreeMap;

use toml_writer::TomlWrite as _;

use crate::alloc_prelude::*;

/// TOML Document serialization buffer
#[derive(Debug)]
pub struct Buffer {
    output: String,
    allocation_pool: String,
    // Original byte offset and table creation order. Implicit parents can share an offset.
    insertions: BTreeMap<(InsertionOffset, Pos), String>,
    next_table_pos: Pos,
}

impl Default for Buffer {
    fn default() -> Self {
        Self {
            output: String::with_capacity(128),
            allocation_pool: String::new(),
            insertions: BTreeMap::new(),
            next_table_pos: Pos::default(),
        }
    }
}

impl Buffer {
    /// Initialize a new serialization buffer
    pub fn new() -> Self {
        Default::default()
    }

    /// Reset the buffer for serializing another document
    pub fn clear(&mut self) {
        self.output.clear();
        self.insertions.clear();
        self.next_table_pos = Pos::default();
    }

    pub(crate) fn root_table(&mut self) -> Table {
        self.new_table(String::new())
    }

    pub(crate) fn child_table(
        &mut self,
        parent: &mut Table,
        write_key: impl FnOnce(&mut String) -> Result<(), super::Error>,
    ) -> Result<Table, super::Error> {
        let encoded_key = self.child_encoded_key(parent, write_key)?;
        Ok(self.new_table(encoded_key))
    }

    pub(crate) fn child_encoded_key(
        &mut self,
        parent: &mut Table,
        write_key: impl FnOnce(&mut String) -> Result<(), super::Error>,
    ) -> Result<String, super::Error> {
        let mut encoded_key = core::mem::take(&mut self.allocation_pool);
        if !parent.encoded_key.is_empty() {
            encoded_key.push_str(&parent.encoded_key);
            encoded_key.key_sep()?;
        }
        write_key(&mut encoded_key)?;
        self.begin_children(parent);
        Ok(encoded_key)
    }

    pub(crate) fn element_table(&mut self, encoded_key: &str) -> Table {
        let mut element_key = core::mem::take(&mut self.allocation_pool);
        element_key.push_str(encoded_key);
        let mut table = self.new_table(element_key);
        table.array = true;
        table
    }

    fn new_table(&mut self, encoded_key: String) -> Table {
        let pos = self.next_table_pos();
        Table {
            encoded_key,
            pos,
            insert_at: None,
            started: false,
            array: false,
        }
    }

    fn next_table_pos(&mut self) -> Pos {
        let pos = self.next_table_pos;
        self.next_table_pos.0 += 1;
        pos
    }

    fn begin_children(&mut self, table: &mut Table) {
        if table.insert_at.is_none() {
            // An array element needs its own header even if it only contains child tables.
            if table.array && !table.started {
                self.ensure_body(table);
            }
            table.insert_at = Some(self.insertion_offset());
        }
    }

    pub(crate) fn ensure_body(&mut self, table: &mut Table) -> &mut String {
        let (dst, offset) = if let Some(offset) = table.insert_at {
            (
                self.insertions.entry((offset, table.pos)).or_default(),
                offset,
            )
        } else {
            let offset = self.insertion_offset();
            (&mut self.output, offset)
        };
        if !table.started {
            if offset != InsertionOffset::start() {
                dst.push('\n');
            }
            table
                .write_header(dst)
                .expect("writing to a String cannot fail");
            table.started = true;
        }
        dst
    }

    fn insertion_offset(&self) -> InsertionOffset {
        InsertionOffset(self.output.len())
    }

    pub(crate) fn push(&mut self, mut table: Table) {
        if required_table(&table) {
            self.ensure_body(&mut table);
        }
        self.recycle_allocation(table.encoded_key);
    }

    pub(crate) fn recycle_allocation(&mut self, mut encoded_key: String) {
        if self.allocation_pool.capacity() < encoded_key.capacity() {
            encoded_key.clear();
            self.allocation_pool = encoded_key;
        }
    }

    pub(crate) fn into_string(mut self) -> String {
        for ((offset, _), mut text) in self.insertions.into_iter().rev() {
            if offset == InsertionOffset::start() && !self.output.is_empty() {
                text.push('\n');
            }
            self.output.insert_str(offset.0, &text);
        }
        self.output
    }
}

impl core::fmt::Display for Buffer {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        let mut start = InsertionOffset::start();
        let mut leading_section = false;
        for (&(offset, _), text) in &self.insertions {
            if leading_section {
                f.write_str("\n")?;
            }
            f.write_str(&self.output[start.0..offset.0])?;
            f.write_str(text)?;
            start = offset;
            leading_section = offset == InsertionOffset::start();
        }
        if leading_section && !self.output.is_empty() {
            f.write_str("\n")?;
        }
        f.write_str(&self.output[start.0..])
    }
}

#[derive(Copy, Clone, Debug, Default, PartialEq, Eq, PartialOrd, Ord)]
struct Pos(usize);

#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
struct InsertionOffset(usize);

impl InsertionOffset {
    const fn start() -> Self {
        Self(0)
    }
}

fn required_table(table: &Table) -> bool {
    // Empty leaf tables are explicit. Empty parents are implied by their children.
    !table.started && table.insert_at.is_none() && !table.encoded_key.is_empty()
}

#[derive(Debug)]
pub(crate) struct Table {
    encoded_key: String,
    pos: Pos,
    // Frozen when the first child starts. Later body entries belong at this boundary.
    insert_at: Option<InsertionOffset>,
    started: bool,
    array: bool,
}

impl Table {
    fn write_header(&self, dst: &mut String) -> core::fmt::Result {
        if !self.encoded_key.is_empty() {
            if self.array {
                dst.open_array_of_tables_header()?;
            } else {
                dst.open_table_header()?;
            }
            dst.push_str(&self.encoded_key);
            if self.array {
                dst.close_array_of_tables_header()?;
            } else {
                dst.close_table_header()?;
            }
            dst.newline()?;
        }
        Ok(())
    }
}
