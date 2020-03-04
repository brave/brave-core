extern crate speedreader;

#[cfg(test)]
mod test {
    #[test]
    fn serialization_works() {
        let mut whitelist = speedreader::whitelist::Whitelist::default();
        whitelist.load_predefined();
        let serialized = whitelist.serialize();
        assert!(serialized.is_ok());
        let serialized = serialized.unwrap();
        assert!(!serialized.is_empty());
    }
}
