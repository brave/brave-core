//! Handling of the wasm `producers` section
//!
//! Specified upstream at
//! https://github.com/WebAssembly/tool-conventions/blob/master/ProducersSection.md

use crate::emit::{Emit, EmitContext};
use crate::error::Result;
use crate::module::Module;

/// Representation of the wasm custom section `producers`
#[derive(Debug, Default)]
pub struct ModuleProducers {
    fields: Vec<Field>,
}

#[derive(Debug)]
struct Field {
    name: String,
    values: Vec<Value>,
}

#[derive(Debug)]
struct Value {
    name: String,
    version: String,
}

impl ModuleProducers {
    /// Adds a new `language` (versioned) to the producers section
    pub fn add_language(&mut self, language: &str, version: &str) {
        self.field("language", language, version);
    }

    /// Adds a new `processed-by` (versioned) to the producers section
    pub fn add_processed_by(&mut self, tool: &str, version: &str) {
        self.field("processed-by", tool, version);
    }

    /// Adds a new `sdk` (versioned) to the producers section
    pub fn add_sdk(&mut self, sdk: &str, version: &str) {
        self.field("sdk", sdk, version);
    }

    fn field(&mut self, field_name: &str, name: &str, version: &str) {
        let new_value = Value {
            name: name.to_string(),
            version: version.to_string(),
        };
        for field in self.fields.iter_mut() {
            if field.name != field_name {
                continue;
            }

            for value in field.values.iter_mut() {
                if value.name == name {
                    *value = new_value;
                    return;
                }
            }
            field.values.push(new_value);
            return;
        }
        self.fields.push(Field {
            name: field_name.to_string(),
            values: vec![new_value],
        })
    }

    /// Clear the producers section of all keys/values
    pub fn clear(&mut self) {
        self.fields.truncate(0);
    }
}

impl Module {
    /// Parse a producers section from the custom section payload specified.
    pub(crate) fn parse_producers_section(
        &mut self,
        data: wasmparser::ProducersSectionReader,
    ) -> Result<()> {
        log::debug!("parse producers section");

        for field in data {
            let field = field?;
            let mut values = Vec::new();
            for value in field.values {
                let value = value?;
                values.push(Value {
                    name: value.name.to_string(),
                    version: value.version.to_string(),
                });
            }
            let name = field.name.to_string();
            self.producers.fields.push(Field { name, values });
        }

        Ok(())
    }
}

impl Emit for ModuleProducers {
    fn emit(&self, cx: &mut EmitContext) {
        log::debug!("emit producers section");
        if self.fields.is_empty() {
            return;
        }
        let mut wasm_producers_section = wasm_encoder::ProducersSection::new();
        for field in &self.fields {
            let mut producers_field = wasm_encoder::ProducersField::new();
            for value in &field.values {
                producers_field.value(&value.name, &value.version);
            }
            wasm_producers_section.field(&field.name, &producers_field);
        }
        cx.wasm_module.section(&wasm_producers_section);
    }
}
