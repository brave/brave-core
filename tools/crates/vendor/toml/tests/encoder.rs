#![cfg(all(feature = "parse", feature = "display"))]
#![allow(dead_code)]

#[derive(Copy, Clone)]
pub(crate) struct Encoder;

impl toml_test_harness::Encoder for Encoder {
    fn name(&self) -> &str {
        "toml"
    }

    fn encode(&self, data: toml_test_harness::Decoded) -> Result<String, toml_test_harness::Error> {
        let value = from_decoded(&data)?;
        let toml::Value::Table(document) = value else {
            return Err(toml_test_harness::Error::new("no root table"));
        };
        let s = toml::to_string(&document).map_err(toml_test_harness::Error::new)?;
        Ok(s)
    }
}

fn from_decoded(
    decoded: &toml_test_harness::Decoded,
) -> Result<toml::Value, toml_test_harness::Error> {
    let value = match decoded {
        toml_test_harness::Decoded::Value(value) => from_decoded_value(value)?,
        toml_test_harness::Decoded::Table(value) => toml::Value::Table(from_table(value)?),
        toml_test_harness::Decoded::Array(value) => toml::Value::Array(from_array(value)?),
    };
    Ok(value)
}

fn from_decoded_value(
    decoded: &toml_test_harness::DecodedValue,
) -> Result<toml::Value, toml_test_harness::Error> {
    match decoded {
        toml_test_harness::DecodedValue::String(value) => Ok(toml::Value::String(value.clone())),
        toml_test_harness::DecodedValue::Integer(value) => value
            .parse::<i64>()
            .map_err(toml_test_harness::Error::new)
            .map(toml::Value::Integer),
        toml_test_harness::DecodedValue::Float(value) => value
            .parse::<f64>()
            .map_err(toml_test_harness::Error::new)
            .map(toml::Value::Float),
        toml_test_harness::DecodedValue::Bool(value) => value
            .parse::<bool>()
            .map_err(toml_test_harness::Error::new)
            .map(toml::Value::Boolean),
        toml_test_harness::DecodedValue::Datetime(value) => value
            .parse::<toml::value::Datetime>()
            .map_err(toml_test_harness::Error::new)
            .map(toml::Value::Datetime),
        toml_test_harness::DecodedValue::DatetimeLocal(value) => value
            .parse::<toml::value::Datetime>()
            .map_err(toml_test_harness::Error::new)
            .map(toml::Value::Datetime),
        toml_test_harness::DecodedValue::DateLocal(value) => value
            .parse::<toml::value::Datetime>()
            .map_err(toml_test_harness::Error::new)
            .map(toml::Value::Datetime),
        toml_test_harness::DecodedValue::TimeLocal(value) => value
            .parse::<toml::value::Datetime>()
            .map_err(toml_test_harness::Error::new)
            .map(toml::Value::Datetime),
    }
}

fn from_table(
    decoded: &std::collections::HashMap<String, toml_test_harness::Decoded>,
) -> Result<toml::value::Table, toml_test_harness::Error> {
    decoded
        .iter()
        .map(|(k, v)| {
            let v = from_decoded(v)?;
            Ok((k.to_owned(), v))
        })
        .collect()
}

fn from_array(
    decoded: &[toml_test_harness::Decoded],
) -> Result<toml::value::Array, toml_test_harness::Error> {
    decoded.iter().map(from_decoded).collect()
}
