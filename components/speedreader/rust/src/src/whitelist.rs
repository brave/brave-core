use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::io::prelude::*;
use flate2::read::GzDecoder;

use crate::speedreader::{SpeedReaderConfig, SpeedReaderError};

const IMAGE_TARGET_WIDTH: u32 = 600;

#[derive(Serialize, Deserialize)]
pub struct Whitelist {
    map: HashMap<String, SpeedReaderConfig>,
}

impl Default for Whitelist {
    fn default() -> Self {
        Whitelist {
            map: HashMap::new(),
        }
    }
}

impl Whitelist {
    pub fn add_configuration(&mut self, config: SpeedReaderConfig) {
        self.map.insert(config.domain.clone(), config);
    }

    pub fn get_configuration(&self, domain: &str) -> Option<&SpeedReaderConfig> {
        if let Some(config) = self.map.get(domain) {
            return Some(config);
        }

        for (i, c) in domain[..domain.len() - 2].char_indices() {
            if c == '.' {
                let subdomain = &domain[i + 1..];
                let maybe_config = self.map.get(subdomain);
                if maybe_config.is_some() {
                    return maybe_config;
                }
            }
        }

        None
    }

    pub fn get_url_rules(&self) -> Vec<String> {
        self.map
            .values()
            .flat_map(|c| c.url_rules.iter().cloned())
            .collect()
    }

    pub fn serialize(&self) -> Result<Vec<u8>, SpeedReaderError> {
        let mut out = Vec::new();
        let j = serde_json::to_string(&self.map.values().collect::<Vec<&SpeedReaderConfig>>())?;
        out.extend_from_slice(j.as_bytes());
        Ok(out)
    }

    pub fn deserialize(serialized: &[u8]) -> Result<Self, SpeedReaderError> {
        let mut gz = GzDecoder::new(serialized);
        let mut s = String::new();
        let read = gz.read_to_string(&mut s);
        if read.is_err() {
            let decoded = std::str::from_utf8(serialized)?;
            s.clear();
            s.push_str(decoded);
        }
        let configurations: Vec<SpeedReaderConfig> = serde_json::from_str(&s)?;
        let mut whitelist = Whitelist::default();
        for config in configurations.into_iter() {
            whitelist.add_configuration(config)
        }
        Ok(whitelist)
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    pub fn default_whitelist_no_config() {
        let whitelist = Whitelist::default();
        assert!(whitelist.map.is_empty());
        let config = whitelist.get_configuration("example.com");
        assert!(config.is_none());
    }

    #[test]
    pub fn get_some_configuration() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        let config = whitelist.get_configuration("example.com");
        assert!(config.is_some());
    }

    #[test]
    pub fn get_some_subdomain_configuration() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        let config = whitelist.get_configuration("www.example.com");
        assert!(config.is_some());
    }

    #[test]
    pub fn url_rules_collected() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.net".to_owned(),
            url_rules: vec![r#"||example.net/article"#.to_owned()],
            declarative_rewrite: None,
        });
        let rules = whitelist.get_url_rules();
        assert_eq!(rules.len(), 3);
    }

    #[test]
    pub fn conflicting_insert_overrides() {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![r#"||example.com/news"#.to_owned()],
            declarative_rewrite: None,
        });
        assert_eq!(whitelist.map.len(), 1);
        let config = whitelist.get_configuration("example.com");
        assert!(config.is_some());
        assert_eq!(
            config.unwrap().url_rules,
            vec!["||example.com/news".to_owned()]
        );
    }
}
