//! Storage and retrieval for redirect and scriptlet resources.

use std::collections::HashMap;

use once_cell::sync::Lazy;
use regex::Regex;
use thiserror::Error;

use super::{MimeType, PermissionMask, Resource, ResourceType};

/// Unified resource storage for both redirects and scriptlets.
#[derive(Default)]
pub struct ResourceStorage {
    /// Stores each resource by its canonical name
    resources: HashMap<String, Resource>,
    /// Stores mappings from aliases to their canonical resource names
    aliases: HashMap<String, String>,
}

impl ResourceStorage {
    /// Convenience constructor that allows building storage for many resources at once. Errors are
    /// silently consumed.
    pub fn from_resources(resources: impl IntoIterator<Item=Resource>) -> Self {
        let mut self_ = Self::default();

        resources.into_iter().for_each(|resource| {
            self_
                .add_resource(resource)
                .unwrap_or_else(|_e| {
                    #[cfg(test)]
                    eprintln!("Failed to add resource: {:?}", _e)
                })
        });

        self_
    }

    /// Adds a resource to storage so that it can be retrieved later.
    pub fn add_resource(&mut self, resource: Resource) -> Result<(), AddResourceError> {
        if let ResourceType::Mime(content_type) = &resource.kind {
            if matches!(content_type, MimeType::FnJavascript) {
                return Err(AddResourceError::FnJavascriptNotSupported);
            }

            if !resource.dependencies.is_empty() && !content_type.supports_dependencies() {
                return Err(AddResourceError::ContentTypeDoesNotSupportDependencies);
            }

            // Ensure the resource contents are valid base64 (and utf8 if applicable)
            let decoded = base64::decode(&resource.content)?;
            if content_type.is_textual() {
                let _ = String::from_utf8(decoded)?;
            }
        }

        for ident in std::iter::once(&resource.name).chain(resource.aliases.iter()) {
            if self.resources.contains_key(ident) || self.aliases.contains_key(ident) {
                return Err(AddResourceError::NameAlreadyAdded);
            }
        }

        resource.aliases.iter().for_each(|alias| {
            self.aliases.insert(alias.clone(), resource.name.clone());
        });
        self.resources.insert(resource.name.clone(), resource);

        Ok(())
    }

    /// Given the contents of a `+js(...)` filter part, return a scriptlet string appropriate for
    /// injection in a page.
    pub fn get_scriptlet_resource(&self, scriptlet_args: &str, filter_permission: PermissionMask) -> Result<String, ScriptletResourceError> {
        let scriptlet_args = parse_scriptlet_args(scriptlet_args);
        if scriptlet_args.is_empty() {
            return Err(ScriptletResourceError::MissingScriptletName);
        }

        let scriptlet_name = with_js_extension(scriptlet_args[0].as_ref());

        let args = &scriptlet_args[1..];
        if args.len() == 1 && args[0].starts_with('{') && args[0].ends_with('}') {
            return Err(ScriptletResourceError::ScriptletArgObjectSyntaxUnsupported);
        }

        let resource = self
            .get_internal_resource(&scriptlet_name)
            .ok_or(ScriptletResourceError::NoMatchingScriptlet)?;

        if !resource.permission.is_injectable_by(filter_permission) {
            return Err(ScriptletResourceError::InsufficientPermissions);
        }

        if !resource.kind.supports_scriptlet_injection() {
            return Err(ScriptletResourceError::ContentTypeNotInjectable);
        }

        let template = String::from_utf8(base64::decode(&resource.content)?)?;

        if template.starts_with("function") {
            // newer function-style resource: pass args using function call syntax
            use itertools::Itertools as _;
            Ok(format!("({})({})", template, args.iter().map(|a| format!("'{}'", a)).join(", ")))
        } else {
            // older template-style resource: replace first instances with args
            Ok(patch_template_scriptlet(template, args))
        }
    }

    /// Get a data-URL formatted resource appropriate for a `$redirect` response.
    pub fn get_redirect_resource(&self, resource_ident: &str) -> Option<String> {
        let resource = self.get_internal_resource(resource_ident);

        resource.and_then(|resource| {
            if !resource.permission.is_default() {
                return None;
            }
            if !resource.kind.supports_redirect() {
                return None;
            }
            if let ResourceType::Mime(mime) = &resource.kind {
                Some(format!("data:{};base64,{}", mime, &resource.content))
            } else {
                None
            }
        })
    }

    /// Gets the resource associated with `resource_ident`, respecting aliases if necessary.
    fn get_internal_resource(&self, resource_ident: &str) -> Option<&Resource> {
        let resource = if let Some(resource) = self.resources.get(resource_ident) {
            Some(resource)
        } else if let Some(canonical_name) = self.aliases.get(resource_ident) {
            self.resources.get(canonical_name)
        } else {
            None
        };

        resource
    }
}

/// Describes failure cases when preparing [`Resource`]s to be used for adblocking.
#[derive(Debug, Error, PartialEq)]
pub enum AddResourceError {
    #[error("invalid base64 content")]
    InvalidBase64Content,
    #[error("invalid utf-8 content")]
    InvalidUtf8Content,
    #[error("resource name already added")]
    NameAlreadyAdded,
    #[error("fn/javascript mime type is not yet supported")]
    FnJavascriptNotSupported,
    #[error("resource content type does not support dependencies")]
    ContentTypeDoesNotSupportDependencies,
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

/// Describes failure cases when attempting to retrieve a resource for scriptlet injection.
#[derive(Debug, Error, PartialEq)]
pub enum ScriptletResourceError {
    #[error("no scriptlet has the provided name")]
    NoMatchingScriptlet,
    #[error("no scriptlet name was provided")]
    MissingScriptletName,
    #[error("object syntax for scriptlet arguments is unsupported")]
    ScriptletArgObjectSyntaxUnsupported,
    #[error("scriptlet content was corrupted")]
    CorruptScriptletContent,
    #[error("resource content type cannot be used for a scriptlet injection")]
    ContentTypeNotInjectable,
    #[error("filter rule is not authorized to inject the intended scriptlet")]
    InsufficientPermissions,
}

impl From<base64::DecodeError> for ScriptletResourceError {
    fn from(_: base64::DecodeError) -> Self {
        Self::CorruptScriptletContent
    }
}

impl From<std::string::FromUtf8Error> for ScriptletResourceError {
    fn from(_: std::string::FromUtf8Error) -> Self {
        Self::CorruptScriptletContent
    }
}

static TEMPLATE_ARGUMENT_RE: [Lazy<Regex>; 9] = [
    Lazy::new(|| template_argument_regex(1)),
    Lazy::new(|| template_argument_regex(2)),
    Lazy::new(|| template_argument_regex(3)),
    Lazy::new(|| template_argument_regex(4)),
    Lazy::new(|| template_argument_regex(5)),
    Lazy::new(|| template_argument_regex(6)),
    Lazy::new(|| template_argument_regex(7)),
    Lazy::new(|| template_argument_regex(8)),
    Lazy::new(|| template_argument_regex(9)),
];

fn template_argument_regex(i: usize) -> Regex {
    Regex::new(&format!(r"\{{\{{{}\}}\}}", i)).unwrap()
}

/// Omit the 0th element of `args` (the scriptlet name) when calling this method.
fn patch_template_scriptlet(mut template: String, args: &[impl AsRef<str>]) -> String {
    // `regex` treats `$` as a special character. Instead, `$$` is interpreted as a literal `$`
    // character.
    args.iter().take(TEMPLATE_ARGUMENT_RE.len()).enumerate().for_each(|(i, arg)| {
        template = TEMPLATE_ARGUMENT_RE[i]
            .replace(&template, arg.as_ref().replace('$', "$$"))
            .to_string();
    });
    template
}

/// Scriptlet injections must be JS resources. However, the `.js` extension may need to be added as
/// a canonicalization step, since it can be omitted in filter rules.
fn with_js_extension(scriptlet_name: &str) -> String {
    if scriptlet_name.ends_with(".js") {
        scriptlet_name.to_string()
    } else {
        format!("{}.js", scriptlet_name)
    }
}

/// Parses the inner contents of a `+js(...)` block into a Vec of its comma-delimited elements.
///
/// A literal comma is produced by the '\,' pattern. Otherwise, all '\', '"', and ''' characters
/// are erased in the resulting arguments.
fn parse_scriptlet_args(args: &str) -> Vec<String> {
    static ESCAPE_SCRIPTLET_ARG_RE: Lazy<Regex> = Lazy::new(|| Regex::new(r#"[\\'"]"#).unwrap());

    // Guarantee that the last character is not a backslash
    let args = args.trim_end_matches('\\');

    let mut args_vec = vec![];
    if args.trim().is_empty() {
        return args_vec;
    }

    let mut after_last_delim = 0;

    let comma_positions = memchr::memchr_iter(b',', args.as_bytes());
    let mut continuation = None;
    for comma_pos in comma_positions.chain(std::iter::once(args.len())) {
        let mut part = &args[after_last_delim..comma_pos];
        let mut is_continuation = false;

        if !part.is_empty() && part.as_bytes()[part.len() - 1] == b'\\' {
            part = &part[0..part.len() - 1];
            is_continuation = true;
        }

        let mut target = if let Some(s) = continuation.take() {
            s
        } else {
            String::new()
        };

        target += part;
        if is_continuation {
            target += ",";
            continuation = Some(target);
        } else {
            args_vec.push(ESCAPE_SCRIPTLET_ARG_RE.replace_all(&target, "\\$0").trim().to_string());
        }

        after_last_delim = comma_pos + 1;
    }

    args_vec
}

#[cfg(test)]
mod redirect_storage_tests {
    use super::*;

    #[test]
    fn get_resource_by_name() {
        let mut storage = ResourceStorage::default();
        storage
            .add_resource(
                Resource::simple("name.js", MimeType::ApplicationJavascript, "resource data"),
            )
            .unwrap();

        assert_eq!(
            storage.get_redirect_resource("name.js"),
            Some(format!("data:application/javascript;base64,{}", base64::encode("resource data"))),
        );
    }

    #[test]
    fn get_resource_by_alias() {
        let mut storage = ResourceStorage::default();
        let mut r = Resource::simple("name.js", MimeType::ApplicationJavascript, "resource data");
        r.aliases.push("alias.js".to_string());
        storage
            .add_resource(r)
            .unwrap();

        assert_eq!(
            storage.get_redirect_resource("alias.js"),
            Some(format!("data:application/javascript;base64,{}", base64::encode("resource data"))),
        );
    }

    #[test]
    fn permissions() {
        let mut storage = ResourceStorage::default();
        let mut r = Resource::simple("name.js", MimeType::ApplicationJavascript, "resource data");
        r.aliases.push("alias.js".to_string());
        r.permission = PermissionMask::from_bits(0b00000001);
        storage
            .add_resource(r)
            .unwrap();

        assert_eq!(
            storage.get_redirect_resource("name.js"),
            None,
        );
        assert_eq!(
            storage.get_redirect_resource("alias.js"),
            None,
        );
    }
}

#[cfg(test)]
mod scriptlet_storage_tests {
    use super::*;

    #[test]
    fn parse_argslist() {
        let args = parse_scriptlet_args("scriptlet, hello world, foobar");
        assert_eq!(args, vec!["scriptlet", "hello world", "foobar"]);
    }

    #[test]
    fn parse_argslist_noargs() {
        let args = parse_scriptlet_args("scriptlet");
        assert_eq!(args, vec!["scriptlet"]);
    }

    #[test]
    fn parse_argslist_empty() {
        let args = parse_scriptlet_args("");
        assert!(args.is_empty());
    }

    #[test]
    fn parse_argslist_commas() {
        let args = parse_scriptlet_args("scriptletname, one\\, two\\, three, four");
        assert_eq!(args, vec!["scriptletname", "one, two, three", "four"]);
    }

    #[test]
    fn parse_argslist_badchars() {
        let args = parse_scriptlet_args(
            r##"scriptlet, "; window.location.href = bad.com; , '; alert("you're\, hacked");    ,    \u\r\l(bad.com) "##,
        );
        assert_eq!(
            args,
            vec![
                r#"scriptlet"#,
                r#"\"; window.location.href = bad.com;"#,
                r#"\'; alert(\"you\'re, hacked\");"#,
                r#"\\u\\r\\l(bad.com)"#
            ]
        );
    }


    #[test]
    fn get_patched_scriptlets() {
        let resources = ResourceStorage::from_resources([
            Resource {
                name: "greet.js".to_string(),
                aliases: vec![],
                kind: ResourceType::Template,
                content: base64::encode("console.log('Hello {{1}}, my name is {{2}}')"),
                dependencies: vec![],
                permission: Default::default(),
            },
            Resource {
                name: "alert.js".to_owned(),
                aliases: vec![],
                kind: ResourceType::Template,
                content: base64::encode("alert('{{1}}')"),
                dependencies: vec![],
                permission: Default::default(),
            },
            Resource {
                name: "blocktimer.js".to_owned(),
                aliases: vec![],
                kind: ResourceType::Template,
                content: base64::encode("setTimeout(blockAds, {{1}})"),
                dependencies: vec![],
                permission: Default::default(),
            },
            Resource {
                name: "null.js".to_owned(),
                aliases: vec![],
                kind: ResourceType::Template,
                content: base64::encode("(()=>{})()"),
                dependencies: vec![],
                permission: Default::default(),
            },
            Resource {
                name: "set-local-storage-item.js".to_owned(),
                aliases: vec![],
                kind: ResourceType::Template,
                content: base64::encode(r#"{{1}} that dollar signs in {{2}} are untouched"#),
                dependencies: vec![],
                permission: Default::default(),
            },
        ]);

        assert_eq!(
            resources.get_scriptlet_resource("greet, world, adblock-rust", Default::default()),
            Ok("console.log('Hello world, my name is adblock-rust')".into())
        );
        assert_eq!(
            resources.get_scriptlet_resource("alert, All systems are go!! ", Default::default()),
            Ok("alert('All systems are go!!')".into())
        );
        assert_eq!(
            resources.get_scriptlet_resource("alert, Uh oh\\, check the logs...", Default::default()),
            Ok("alert('Uh oh, check the logs...')".into())
        );
        assert_eq!(
            resources.get_scriptlet_resource(r#"alert, this has "quotes""#, Default::default()),
            Ok(r#"alert('this has \"quotes\"')"#.into())
        );
        assert_eq!(
            resources.get_scriptlet_resource("blocktimer, 3000", Default::default()),
            Ok("setTimeout(blockAds, 3000)".into())
        );
        assert_eq!(resources.get_scriptlet_resource("null", Default::default()), Ok("(()=>{})()".into()));
        assert_eq!(
            resources.get_scriptlet_resource("null, null", Default::default()),
            Ok("(()=>{})()".into())
        );
        assert_eq!(
            resources.get_scriptlet_resource("greet, everybody", Default::default()),
            Ok("console.log('Hello everybody, my name is {{2}}')".into())
        );

        assert_eq!(
            resources.get_scriptlet_resource("unit-testing", Default::default()),
            Err(ScriptletResourceError::NoMatchingScriptlet)
        );
        assert_eq!(
            resources.get_scriptlet_resource("", Default::default()),
            Err(ScriptletResourceError::MissingScriptletName)
        );

        assert_eq!(
            resources.get_scriptlet_resource("set-local-storage-item, Test, $remove$", Default::default()),
            Ok("Test that dollar signs in $remove$ are untouched".into()),
        );
    }

    #[test]
    fn parse_template_file_format() {
        let resources = ResourceStorage::from_resources([
            Resource {
                name: "abort-current-inline-script.js".into(),
                aliases: vec!["acis.js".into()],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("(function() {alert(\"hi\");})();"),
                dependencies: vec![],
                permission: Default::default(),
            },
            Resource {
                name: "abort-on-property-read.js".into(),
                aliases: vec!["aopr.js".into()],
                kind: ResourceType::Template,
                content: base64::encode("(function() {confirm(\"Do you want to {{1}}?\");})();"),
                dependencies: vec![],
                permission: Default::default(),
            },
            Resource {
                name: "googletagservices_gpt.js".into(),
                aliases: vec!["googletagservices.com/gpt.js".into(), "googletagservices-gpt".into()],
                kind: ResourceType::Template,
                content: base64::encode("function(a1 = '', a2 = '') {console.log(a1, a2)}"),
                dependencies: vec![],
                permission: Default::default(),
            },
        ]);

        assert_eq!(
            resources.get_scriptlet_resource("aopr, code", Default::default()),
            Ok("(function() {confirm(\"Do you want to code?\");})();".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource("abort-on-property-read, write tests", Default::default()),
            Ok("(function() {confirm(\"Do you want to write tests?\");})();".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource("abort-on-property-read.js, block advertisements", Default::default()),
            Ok("(function() {confirm(\"Do you want to block advertisements?\");})();".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource("acis", Default::default()),
            Ok("(function() {alert(\"hi\");})();".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource("acis.js", Default::default()),
            Ok("(function() {alert(\"hi\");})();".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource("googletagservices_gpt.js", Default::default()),
            Ok("(function(a1 = '', a2 = '') {console.log(a1, a2)})()".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource("googletagservices_gpt, test1", Default::default()),
            Ok("(function(a1 = '', a2 = '') {console.log(a1, a2)})('test1')".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource("googletagservices.com/gpt, test1, test2", Default::default()),
            Ok("(function(a1 = '', a2 = '') {console.log(a1, a2)})('test1', 'test2')".to_owned()),
        );

        assert_eq!(
            resources.get_scriptlet_resource(r#"googletagservices.com/gpt.js, t"es't1, $te\st2$"#, Default::default()),
            Ok(r#"(function(a1 = '', a2 = '') {console.log(a1, a2)})('t\"es\'t1', '$te\\st2$')"#.to_owned()),
        );

        // The alias does not have a `.js` extension, so it cannot be used for a scriptlet
        // injection (only as a redirect resource).
        assert_eq!(
            resources.get_scriptlet_resource(r#"googletagservices-gpt, t"es't1, te\st2"#, Default::default()),
            Err(ScriptletResourceError::NoMatchingScriptlet),
        );

        // Object-style injection
        assert_eq!(
            resources.get_scriptlet_resource(r#"googletagservices.com/gpt, { "test": true }"#, Default::default()),
            Err(ScriptletResourceError::ScriptletArgObjectSyntaxUnsupported),
        );
    }

    /// Currently, only 9 template arguments are supported - but reaching that limit should not
    /// cause a panic.
    #[test]
    fn patch_argslist_many_args() {
        let resources = ResourceStorage::from_resources([
            Resource {
                name: "abort-current-script.js".into(),
                aliases: vec!["acs.js".into()],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("{{1}} {{2}} {{3}} {{4}} {{5}} {{6}} {{7}} {{8}} {{9}} {{10}} {{11}} {{12}}"),
                dependencies: vec![],
                permission: Default::default(),
            },
        ]);

        let args = parse_scriptlet_args("acs, this, probably, is, going, to, break, brave, and, crash, it, instead, of, ignoring, it");
        assert_eq!(args, vec!["acs", "this", "probably", "is", "going", "to", "break", "brave", "and", "crash", "it", "instead", "of", "ignoring", "it"]);

        assert_eq!(
            resources.get_scriptlet_resource("acs, this, probably, is, going, to, break, brave, and, crash, it, instead, of, ignoring, it", Default::default()),
            Ok("this probably is going to break brave and crash {{10}} {{11}} {{12}}".to_string()),
        );
    }

    #[test]
    fn permissions() {
        const PERM0: PermissionMask = PermissionMask::from_bits(0b00000001);
        const PERM1: PermissionMask = PermissionMask::from_bits(0b00000010);
        const PERM10: PermissionMask = PermissionMask::from_bits(0b00000011);
        let resources = ResourceStorage::from_resources([
            Resource::simple("default-perms.js", MimeType::ApplicationJavascript, "default-perms"),
            Resource {
                name: "perm0.js".into(),
                aliases: vec!["0.js".to_string()],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("perm0"),
                dependencies: vec![],
                permission: PERM0,
            },
            Resource {
                name: "perm1.js".into(),
                aliases: vec!["1.js".to_string()],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("perm1"),
                dependencies: vec![],
                permission: PERM1,
            },
            Resource {
                name: "perm10.js".into(),
                aliases: vec!["10.js".to_string()],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("perm10"),
                dependencies: vec![],
                permission: PERM10,
            },
        ]);

        fn test_perm(resources: &ResourceStorage, perm: PermissionMask, expect_ok: &[&str], expect_fail: &[&str]) {
            for ident in expect_ok {
                if ident.len() > 2 {
                    assert_eq!(
                        resources.get_scriptlet_resource(ident, perm),
                        Ok(ident.to_string()),
                    );
                } else {
                    assert_eq!(
                        resources.get_scriptlet_resource(ident, perm),
                        Ok(format!("perm{}", ident)),
                    );
                }
            }

            for ident in expect_fail {
                assert_eq!(
                    resources.get_scriptlet_resource(ident, perm),
                    Err(ScriptletResourceError::InsufficientPermissions),
                );
            }
        }

        test_perm(&resources, Default::default(), &["default-perms"], &["perm0", "perm1", "perm10", "0", "1", "10"]);
        test_perm(&resources, PERM0, &["default-perms", "perm0", "0"], &["perm1", "perm10", "1", "10"]);
        test_perm(&resources, PERM1, &["default-perms", "perm1", "1"], &["perm0", "perm10", "0", "10"]);
        test_perm(&resources, PERM10, &["default-perms", "perm0", "perm1", "perm10", "0", "1", "10"], &[]);
    }
}
