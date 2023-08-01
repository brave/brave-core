//! In adblocking terms, [`Resource`]s are special placeholder scripts, images,
//! video files, etc. that can be returned as drop-in replacements for harmful
//! equivalents from remote servers. Resources also encompass scriptlets, which
//! can be injected into pages to inhibit malicious behavior.
//!
//! If the `resource-assembler` feature is enabled, the
#![cfg_attr(not(feature = "resource-assembler"), doc="`resource_assembler`")]
#![cfg_attr(feature = "resource-assembler", doc="[`resource_assembler`]")]
//! module will assist with the construction of [`Resource`]s directly from the uBlock Origin
//! project.

#[cfg(feature = "resource-assembler")]
pub mod resource_assembler;

mod scriptlet_resource_storage;
pub(crate) use scriptlet_resource_storage::ScriptletResourceStorage;

use memchr::memrchr as find_char_reverse;
use serde::{Deserialize, Serialize};
use thiserror::Error;
use std::collections::HashMap;

/// Struct representing a resource that can be used by an adblocking engine.
///
/// - `name`: Represents the primary name of the resource, often a filename
///
/// - `aliases`: Represents secondary names that can be used to access the resource
///
/// - `kind`: How to interpret the resource data within `content`
///
/// - `content`: The resource data, encoded using standard base64 configuration
#[derive(Serialize, Deserialize)]
pub struct Resource {
    pub name: String,
    pub aliases: Vec<String>,
    pub kind: ResourceType,
    pub content: String,
}

/// Different ways that the data within the `content` field of a `Resource` can be interpreted.
///
/// - `Mime(type)` - interpret the data according to the MIME type represented by `type`
///
/// - `Template` - interpret the data as a Javascript scriptlet template, with embedded template
/// parameters in the form of `{{1}}`, `{{2}}`, etc.
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
#[serde(rename_all = "lowercase")]
pub enum ResourceType {
    Mime(MimeType),
    Template,
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq)]
#[serde(into = "String")]
#[serde(from = "std::borrow::Cow<'static, str>")]
pub enum MimeType {
    TextCss,
    ImageGif,
    TextHtml,
    ApplicationJavascript,
    AudioMp3,
    VideoMp4,
    ImagePng,
    TextPlain,
    TextXml,
    Unknown,
}

#[derive(Debug, Error, PartialEq)]
pub enum AddResourceError {
    #[error("invalid base64 content")]
    InvalidBase64Content,
    #[error("invalid utf-8 content")]
    InvalidUtf8Content,
}

impl From<base64::DecodeError> for AddResourceError {
    fn from(_: base64::DecodeError) -> Self {
        AddResourceError::InvalidBase64Content
    }
}

impl From<std::string::FromUtf8Error> for AddResourceError {
    fn from(_: std::string::FromUtf8Error) -> Self {
        AddResourceError::InvalidUtf8Content
    }
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub struct RedirectResource {
    pub content_type: String,
    pub data: String,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Default)]
pub struct RedirectResourceStorage {
    #[serde(serialize_with = "crate::data_format::utils::stabilize_hashmap_serialization")]
    pub resources: HashMap<String, RedirectResource>,
}

impl MimeType {
    /// Infers a resource's MIME type according to the extension of its path
    pub fn from_extension(resource_path: &str) -> Self {
        if let Some(extension_index) = find_char_reverse(b'.', resource_path.as_bytes()) {
            match &resource_path[extension_index + 1..] {
                "css" => MimeType::TextCss,
                "gif" => MimeType::ImageGif,
                "html" => MimeType::TextHtml,
                "js" => MimeType::ApplicationJavascript,
                "mp3" => MimeType::AudioMp3,
                "mp4" => MimeType::VideoMp4,
                "png" => MimeType::ImagePng,
                "txt" => MimeType::TextPlain,
                "xml" => MimeType::TextXml,
                _ => {
                    #[cfg(test)]
                    eprintln!("Unrecognized file extension on: {:?}", resource_path);
                    MimeType::Unknown
                }
            }
        } else {
            MimeType::Unknown
        }
    }
}

impl RedirectResourceStorage {
    pub fn from_resources(resources: &[Resource]) -> Self {
        let mut redirectable_resources: HashMap<String, RedirectResource> = HashMap::new();

        resources
            .iter()
            .filter_map(|descriptor| {
                if let ResourceType::Mime(ref content_type) = descriptor.kind {
                    let resource = RedirectResource {
                        content_type: content_type.clone().into(),
                        data: descriptor.content.to_owned(),
                    };
                    Some((
                        descriptor.name.to_owned(),
                        descriptor.aliases.to_owned(),
                        resource,
                    ))
                } else {
                    None
                }
            })
            .for_each(|(name, res_aliases, resource)| {
                res_aliases.iter().for_each(|alias| {
                    redirectable_resources.insert(alias.to_owned(), resource.clone());
                });
                redirectable_resources.insert(name, resource);
            });

        Self {
            resources: redirectable_resources,
        }
    }

    pub fn get_resource(&self, name: &str) -> Option<&RedirectResource> {
        self.resources.get(name)
    }

    /// Adds a resource. Only has an effect for mimetyped scriptlets.
    pub fn add_resource(&mut self, resource: &Resource) -> Result<(), AddResourceError> {
        if let ResourceType::Mime(ref content_type) = resource.kind {
            // Ensure the resource contents are valid base64
            let decoded = base64::decode(&resource.content)?;
            match content_type {
                // Ensure any text contents are also valid utf8
                MimeType::ApplicationJavascript
                | MimeType::TextPlain
                | MimeType::TextHtml
                | MimeType::TextXml => {
                    let _ = String::from_utf8(decoded)?;
                }
                _ => (),
            }

            let name = resource.name.to_owned();
            let redirect_resource = RedirectResource {
                content_type: content_type.clone().into(),
                data: resource.content.to_owned(),
            };
            resource.aliases.iter().for_each(|alias| {
                self.resources
                    .insert(alias.to_owned(), redirect_resource.clone());
            });
            self.resources.insert(name, redirect_resource);
        }
        Ok(())
    }
}

impl From<std::borrow::Cow<'static, str>> for MimeType {
    fn from(v: std::borrow::Cow<'static, str>) -> Self {
        v.as_ref().into()
    }
}

impl From<&str> for MimeType {
    fn from(v: &str) -> Self {
        match v {
            "text/css" => MimeType::TextCss,
            "image/gif" => MimeType::ImageGif,
            "text/html" => MimeType::TextHtml,
            "application/javascript" => MimeType::ApplicationJavascript,
            "audio/mp3" => MimeType::AudioMp3,
            "video/mp4" => MimeType::VideoMp4,
            "image/png" => MimeType::ImagePng,
            "text/plain" => MimeType::TextPlain,
            "text/xml" => MimeType::TextXml,
            _ => MimeType::Unknown,
        }
    }
}

impl From<MimeType> for String {
    fn from(v: MimeType) -> Self {
        match v {
            MimeType::TextCss => "text/css",
            MimeType::ImageGif => "image/gif",
            MimeType::TextHtml => "text/html",
            MimeType::ApplicationJavascript => "application/javascript",
            MimeType::AudioMp3 => "audio/mp3",
            MimeType::VideoMp4 => "video/mp4",
            MimeType::ImagePng => "image/png",
            MimeType::TextPlain => "text/plain",
            MimeType::TextXml => "text/xml",
            MimeType::Unknown => "application/octet-stream",
        }
        .to_owned()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn get_resource_by_name() {
        let mut storage = RedirectResourceStorage::default();
        storage
            .add_resource(&Resource {
                name: "name.js".to_owned(),
                aliases: vec![],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("resource data"),
            })
            .unwrap();

        assert_eq!(
            storage.get_resource("name.js"),
            Some(&RedirectResource {
                content_type: "application/javascript".to_owned(),
                data: base64::encode("resource data"),
            })
        );
    }

    #[test]
    fn get_resource_by_alias() {
        let mut storage = RedirectResourceStorage::default();
        storage
            .add_resource(&Resource {
                name: "name.js".to_owned(),
                aliases: vec!["alias.js".to_owned()],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("resource data"),
            })
            .unwrap();

        assert_eq!(
            storage.get_resource("alias.js"),
            Some(&RedirectResource {
                content_type: "application/javascript".to_owned(),
                data: base64::encode("resource data"),
            })
        );
    }
}
