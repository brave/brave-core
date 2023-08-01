use std::collections::HashMap;

use once_cell::sync::Lazy;
use regex::Regex;
use serde::{Deserialize, Serialize};

use crate::resources::{AddResourceError, MimeType, Resource, ResourceType};

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

#[derive(Debug, PartialEq)]
pub enum ScriptletResourceError {
    NoMatchingScriptlet,
    MissingScriptletName,
}

#[derive(Clone, Deserialize, Serialize)]
pub struct ScriptletResource {
    scriptlet: String,
}

impl ScriptletResource {
    /// Omit the 0th element of `args` (the scriptlet name) when calling this method.
    fn patch(&self, args: &[impl AsRef<str>]) -> String {
        let mut scriptlet = self.scriptlet.to_owned();
        // `regex` treats `$` as a special character. Instead, `$$` is interpreted as a literal `$`
        // character.
        args.iter().enumerate().for_each(|(i, arg)| {
            scriptlet = TEMPLATE_ARGUMENT_RE[i]
                .replace(&scriptlet, arg.as_ref().replace('$', "$$"))
                .to_string();
        });
        scriptlet
    }
}

#[derive(Default, Deserialize, Serialize)]
pub struct ScriptletResourceStorage {
    #[serde(serialize_with = "crate::data_format::utils::stabilize_hashmap_serialization")]
    resources: HashMap<String, ScriptletResource>,
}

impl ScriptletResourceStorage {
    /// Convenience constructor that allows building storage for many resources at once, printing
    /// any errors that occur.
    #[cfg(test)]
    pub fn from_resources(resources: &[Resource]) -> Self {
        let mut self_ = Self::default();

        resources.iter().for_each(|resource| {
            self_
                .add_resource(&resource)
                .unwrap_or_else(|_e| eprintln!("Failed to add resource: {:?}", _e))
        });

        self_
    }

    /// Adds a resource. Only has an effect for application/javascript mimetypes and template
    /// scriptlets.
    pub fn add_resource(&mut self, resource: &Resource) -> Result<(), AddResourceError> {
        let scriptlet = match resource.kind {
            ResourceType::Mime(MimeType::ApplicationJavascript) | ResourceType::Template => {
                let scriptlet = ScriptletResource {
                    scriptlet: String::from_utf8(base64::decode(&resource.content)?)?,
                };
                Some((
                    resource.name.to_owned(),
                    resource.aliases.to_owned(),
                    scriptlet,
                ))
            }
            _ => None,
        };

        if let Some((name, res_aliases, resource)) = scriptlet {
            res_aliases.iter().for_each(|alias| {
                self.resources
                    .insert(without_js_extension(alias).to_owned(), resource.clone());
            });
            self.resources
                .insert(without_js_extension(&name).to_owned(), resource);
        };

        Ok(())
    }

    pub fn get_scriptlet(&self, scriptlet_args: &str) -> Result<String, ScriptletResourceError> {
        let scriptlet_args = parse_scriptlet_args(scriptlet_args);
        if scriptlet_args.is_empty() {
            return Err(ScriptletResourceError::MissingScriptletName);
        }
        let scriptlet_name = without_js_extension(scriptlet_args[0].as_ref());
        let args = &scriptlet_args[1..];
        let template = self
            .resources
            .get(scriptlet_name)
            .ok_or(ScriptletResourceError::NoMatchingScriptlet)?;

        Ok(template.patch(args))
    }
}

fn without_js_extension(scriptlet_name: &str) -> &str {
    if let Some(stripped) = scriptlet_name.strip_suffix(".js") {
        stripped
    } else {
        scriptlet_name
    }
}

/// Parses the inner contents of a `+js(...)` block into a Vec of its comma-delimited elements.
///
/// A literal comma is produced by the '\,' pattern. Otherwise, all '\', '"', and ''' characters
/// are erased in the resulting arguments.
pub fn parse_scriptlet_args(args: &str) -> Vec<String> {
    static ESCAPE_SCRIPTLET_ARG_RE: Lazy<Regex> = Lazy::new(|| Regex::new(r#"[\\'"]"#).unwrap());

    // Guarantee that the last character is not a backslash
    let args = args.trim_end_matches('\\');

    let mut args_vec = vec![];
    if args.trim().len() == 0 {
        return args_vec;
    }

    let mut after_last_delim = 0;

    let comma_positions = memchr::memchr_iter(b',', args.as_bytes());
    let mut continuation = None;
    for comma_pos in comma_positions.chain(std::iter::once(args.len())) {
        let mut part = &args[after_last_delim..comma_pos];
        let mut is_continuation = false;

        if part.len() > 0 && part.as_bytes()[part.len() - 1] == b'\\' {
            part = &part[0..part.len() - 1];
            is_continuation = true;
        }

        let mut target = if let Some(s) = continuation.take() {
            String::from(s)
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
mod tests {
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
        let mut resources = HashMap::new();
        resources.insert(
            "greet".to_owned(),
            ScriptletResource {
                scriptlet: "console.log('Hello {{1}}, my name is {{2}}')".to_owned(),
            },
        );
        resources.insert(
            "alert".to_owned(),
            ScriptletResource {
                scriptlet: "alert('{{1}}')".to_owned(),
            },
        );
        resources.insert(
            "blocktimer".to_owned(),
            ScriptletResource {
                scriptlet: "setTimeout(blockAds, {{1}})".to_owned(),
            },
        );
        resources.insert(
            "null".to_owned(),
            ScriptletResource {
                scriptlet: "(()=>{})()".to_owned(),
            },
        );
        resources.insert(
            "set-local-storage-item".to_owned(),
            ScriptletResource {
                scriptlet: r#"{{1}} that dollar signs in {{2}} are untouched"#.to_owned(),
            }
        );
        let scriptlets = ScriptletResourceStorage { resources };

        assert_eq!(
            scriptlets.get_scriptlet("greet, world, adblock-rust"),
            Ok("console.log('Hello world, my name is adblock-rust')".into())
        );
        assert_eq!(
            scriptlets.get_scriptlet("alert, All systems are go!! "),
            Ok("alert('All systems are go!!')".into())
        );
        assert_eq!(
            scriptlets.get_scriptlet("alert, Uh oh\\, check the logs..."),
            Ok("alert('Uh oh, check the logs...')".into())
        );
        assert_eq!(
            scriptlets.get_scriptlet(r#"alert, this has "quotes""#),
            Ok(r#"alert('this has \"quotes\"')"#.into())
        );
        assert_eq!(
            scriptlets.get_scriptlet("blocktimer, 3000"),
            Ok("setTimeout(blockAds, 3000)".into())
        );
        assert_eq!(scriptlets.get_scriptlet("null"), Ok("(()=>{})()".into()));
        assert_eq!(
            scriptlets.get_scriptlet("null, null"),
            Ok("(()=>{})()".into())
        );
        assert_eq!(
            scriptlets.get_scriptlet("greet, everybody"),
            Ok("console.log('Hello everybody, my name is {{2}}')".into())
        );

        assert_eq!(
            scriptlets.get_scriptlet("unit-testing"),
            Err(ScriptletResourceError::NoMatchingScriptlet)
        );
        assert_eq!(
            scriptlets.get_scriptlet(""),
            Err(ScriptletResourceError::MissingScriptletName)
        );

        assert_eq!(
            scriptlets.get_scriptlet("set-local-storage-item, Test, $remove$"),
            Ok("Test that dollar signs in $remove$ are untouched".into()),
        );
    }

    #[test]
    fn parse_template_file_format() {
        let scriptlets = ScriptletResourceStorage::from_resources(&[
            Resource {
                name: "abort-current-inline-script.js".into(),
                aliases: vec!["acis.js".into()],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("(function() {alert(\"hi\");})();"),
            },
            Resource {
                name: "abort-on-property-read.js".into(),
                aliases: vec!["aopr".into()],
                kind: ResourceType::Template,
                content: base64::encode("(function() {confirm(\"Do you want to {{1}}?\");})();"),
            },
        ]);

        assert_eq!(
            scriptlets.get_scriptlet("aopr, code"),
            Ok("(function() {confirm(\"Do you want to code?\");})();".to_owned()),
        );

        assert_eq!(
            scriptlets.get_scriptlet("abort-on-property-read, write tests"),
            Ok("(function() {confirm(\"Do you want to write tests?\");})();".to_owned()),
        );

        assert_eq!(
            scriptlets.get_scriptlet("abort-on-property-read.js, block advertisements"),
            Ok("(function() {confirm(\"Do you want to block advertisements?\");})();".to_owned()),
        );

        assert_eq!(
            scriptlets.get_scriptlet("acis"),
            Ok("(function() {alert(\"hi\");})();".to_owned()),
        );

        assert_eq!(
            scriptlets.get_scriptlet("acis.js"),
            Ok("(function() {alert(\"hi\");})();".to_owned()),
        );
    }
}
