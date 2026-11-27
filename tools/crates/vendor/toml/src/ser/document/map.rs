use toml_writer::TomlWrite as _;

use super::Buffer;
use super::Error;
use super::SerializationStrategy;
use super::Serializer;
use super::Table;
use super::array_of_tables::ArrayOfTablesSerializer;
use super::style::Style;
use super::value::KeySerializer;
use super::value::ValueSerializer;
use crate::alloc_prelude::*;

#[doc(hidden)]
pub struct SerializeDocumentTable<'d> {
    buf: &'d mut Buffer,
    table: Table,
    encoded_key: Option<String>,
    style: Style,
}

impl<'d> SerializeDocumentTable<'d> {
    pub(crate) fn map(buf: &'d mut Buffer, table: Table, style: Style) -> Result<Self, Error> {
        Ok(Self {
            buf,
            table,
            encoded_key: None,
            style,
        })
    }

    fn end(self) -> Result<&'d mut Buffer, Error> {
        self.buf.push(self.table);
        Ok(self.buf)
    }

    fn write_entry<T>(
        &mut self,
        write_key: impl FnOnce(&mut String) -> Result<(), Error>,
        value: &T,
    ) -> Result<(), Error>
    where
        T: serde_core::ser::Serialize + ?Sized,
    {
        match SerializationStrategy::from(value) {
            SerializationStrategy::Value => {
                let dst = self.buf.ensure_body(&mut self.table);

                write_key(dst)?;
                dst.space()?;
                dst.keyval_sep()?;
                dst.space()?;
                let value_serializer = ValueSerializer::with_style(dst, self.style);
                let dst = value.serialize(value_serializer)?;
                dst.newline()?;
            }
            SerializationStrategy::ArrayOfTables => {
                let encoded_key = self.buf.child_encoded_key(&mut self.table, write_key)?;
                let value_serializer =
                    ArrayOfTablesSerializer::new(self.buf, encoded_key, self.style);
                value.serialize(value_serializer)?;
            }
            SerializationStrategy::Table | SerializationStrategy::Unknown => {
                let child = self.buf.child_table(&mut self.table, write_key)?;
                let value_serializer = Serializer::with_table(self.buf, child, self.style);
                value.serialize(value_serializer)?;
            }
            SerializationStrategy::Skip => {
                // Validate the key, then silently drop the key-value pair.
                write_key(&mut String::new())?;
            }
        }
        Ok(())
    }
}

impl<'d> serde_core::ser::SerializeMap for SerializeDocumentTable<'d> {
    type Ok = &'d mut Buffer;
    type Error = Error;

    fn serialize_key<T>(&mut self, input: &T) -> Result<(), Self::Error>
    where
        T: serde_core::ser::Serialize + ?Sized,
    {
        let mut encoded_key = String::new();
        input.serialize(KeySerializer {
            dst: &mut encoded_key,
        })?;
        self.encoded_key = Some(encoded_key);
        Ok(())
    }

    fn serialize_value<T>(&mut self, value: &T) -> Result<(), Self::Error>
    where
        T: serde_core::ser::Serialize + ?Sized,
    {
        let encoded_key = self
            .encoded_key
            .take()
            .expect("always called after `serialize_key`");
        self.write_entry(
            |dst| {
                if dst.is_empty() {
                    *dst = encoded_key;
                } else {
                    dst.push_str(&encoded_key);
                }
                Ok(())
            },
            value,
        )
    }

    fn serialize_entry<K, V>(&mut self, key: &K, value: &V) -> Result<(), Self::Error>
    where
        K: serde_core::ser::Serialize + ?Sized,
        V: serde_core::ser::Serialize + ?Sized,
    {
        self.write_entry(|dst| key.serialize(KeySerializer { dst }), value)
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        self.end()
    }
}

impl<'d> serde_core::ser::SerializeStruct for SerializeDocumentTable<'d> {
    type Ok = &'d mut Buffer;
    type Error = Error;

    fn serialize_field<T>(&mut self, key: &'static str, value: &T) -> Result<(), Self::Error>
    where
        T: serde_core::ser::Serialize + ?Sized,
    {
        match SerializationStrategy::from(value) {
            SerializationStrategy::Value => {
                let dst = self.buf.ensure_body(&mut self.table);

                dst.key(key)?;
                dst.space()?;
                dst.keyval_sep()?;
                dst.space()?;
                let value_serializer = ValueSerializer::with_style(dst, self.style);
                let dst = value.serialize(value_serializer)?;
                dst.newline()?;
            }
            SerializationStrategy::ArrayOfTables => {
                let encoded_key = self
                    .buf
                    .child_encoded_key(&mut self.table, |dst| dst.key(key).map_err(Error::from))?;
                let value_serializer =
                    ArrayOfTablesSerializer::new(self.buf, encoded_key, self.style);
                value.serialize(value_serializer)?;
            }
            SerializationStrategy::Table | SerializationStrategy::Unknown => {
                let child = self
                    .buf
                    .child_table(&mut self.table, |dst| dst.key(key).map_err(Error::from))?;
                let value_serializer = Serializer::with_table(self.buf, child, self.style);
                value.serialize(value_serializer)?;
            }
            SerializationStrategy::Skip => {
                // silently drop these key-value pairs
            }
        }

        Ok(())
    }

    fn end(self) -> Result<Self::Ok, Self::Error> {
        self.end()
    }
}

impl<'d> serde_core::ser::SerializeStructVariant for SerializeDocumentTable<'d> {
    type Ok = &'d mut Buffer;
    type Error = Error;

    #[inline]
    fn serialize_field<T>(&mut self, key: &'static str, value: &T) -> Result<(), Self::Error>
    where
        T: serde_core::ser::Serialize + ?Sized,
    {
        serde_core::ser::SerializeStruct::serialize_field(self, key, value)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        self.end()
    }
}
