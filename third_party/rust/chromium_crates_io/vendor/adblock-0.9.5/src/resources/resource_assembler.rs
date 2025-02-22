//! Contains methods useful for building [`Resource`] descriptors from resources directly from
//! files in the uBlock Origin repository.

use crate::resources::{MimeType, Resource, ResourceType};
use memchr::memmem;
use once_cell::sync::Lazy;
use regex::Regex;
use std::fs::File;
use std::io::Read;
use std::path::Path;

static TOP_COMMENT_RE: Lazy<Regex> = Lazy::new(|| Regex::new(r#"^/\*[\S\s]+?\n\*/\s*"#).unwrap());
static NON_EMPTY_LINE_RE: Lazy<Regex> = Lazy::new(|| Regex::new(r#"\S"#).unwrap());

/// Represents a single entry of the `Map` from uBlock Origin's `redirect-resources.js`.
struct ResourceProperties {
    /// The name of a resource, corresponding to its path in the `web_accessible_resources`
    /// directory
    name: String,
    /// A list of optional additional names that can be used to reference the resource
    alias: Vec<String>,
    /// Either `"text"` or `"blob"`, but is currently unused in `adblock-rust`. Within uBlock
    /// Origin, it's used to prevent text files from being encoded in base64 in a data URL.
    #[allow(unused)]
    data: Option<String>,
}

/// The deserializable represenation of the `alias` field of a resource's properties, which can
/// either be a single string or a list of strings.
#[derive(serde::Deserialize)]
#[serde(untagged)]
enum ResourceAliasField {
    SingleString(String),
    ListOfStrings(Vec<String>),
}

impl ResourceAliasField {
    fn to_vec(self) -> Vec<String> {
        match self {
            Self::SingleString(s) => vec![s],
            Self::ListOfStrings(l) => l,
        }
    }
}

/// Directly deserializable representation of a resource's properties from `redirect-resources.js`.
#[derive(serde::Deserialize)]
struct JsResourceProperties {
    #[serde(default)]
    alias: Option<ResourceAliasField>,
    #[serde(default)]
    data: Option<String>,
    #[serde(default)]
    params: Option<Vec<String>>,
}

/// Maps the name of the resource to its properties in a 2-element tuple.
type JsResourceEntry = (String, JsResourceProperties);

const REDIRECTABLE_RESOURCES_DECLARATION: &str = "export default new Map([";
//  ]);
static MAP_END_RE: Lazy<Regex> = Lazy::new(|| Regex::new(r#"^\s*\]\s*\)"#).unwrap());

static TRAILING_COMMA_RE: Lazy<Regex> = Lazy::new(|| Regex::new(r#",([\],\}])"#).unwrap());
static UNQUOTED_FIELD_RE: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"([\{,])([a-zA-Z][a-zA-Z0-9_]*):"#).unwrap());
// Avoid matching a starting `/*` inside a string
static TRAILING_BLOCK_COMMENT_RE: Lazy<Regex> =
    Lazy::new(|| Regex::new(r#"\s*/\*[^'"]*\*/\s*$"#).unwrap());

/// Reads data from a a file in the format of uBlock Origin's `redirect-resources.js` file to
/// determine the files in the `web_accessible_resources` directory, as well as any of their
/// aliases.
///
/// This is read from the exported `Map`.
fn read_redirectable_resource_mapping(mapfile_data: &str) -> Vec<ResourceProperties> {
    // This isn't bulletproof, but it should handle the historical versions of the mapping
    // correctly, and having a strict JSON parser should catch any unexpected format changes. Plus,
    // it prevents dependending on a full JS engine.

    // Extract just the map. It's between REDIRECTABLE_RESOURCES_DECLARATION and MAP_END_RE.
    let mut map: String = mapfile_data
        .lines()
        .skip_while(|line| *line != REDIRECTABLE_RESOURCES_DECLARATION)
        .take_while(|line| !MAP_END_RE.is_match(line))
        // Strip any trailing comments from each line.
        .map(|line| {
            if let Some(i) = memmem::find(line.as_bytes(), b"//") {
                &line[..i]
            } else {
                line
            }
        })
        .map(|line| {
            TRAILING_BLOCK_COMMENT_RE.replace_all(line, "")
        })
        // Remove all newlines from the entire string.
        .fold(String::new(), |s, line| s + &line);

    // Add back the final square brace that was omitted above as part of MAP_END_RE.
    map.push(']');

    // Trim out the beginning `export default new Map(`.
    // Also, replace all single quote characters with double quotes.
    assert!(map.starts_with(REDIRECTABLE_RESOURCES_DECLARATION));
    map = map[REDIRECTABLE_RESOURCES_DECLARATION.len() - 1..].replace('\'', "\"");

    // Remove all whitespace from the entire string.
    map.retain(|c| !c.is_whitespace());

    // Replace all matches for `,]` or `,}` with `]` or `}`, respectively.
    map = TRAILING_COMMA_RE
        .replace_all(&map, |caps: &regex::Captures| caps[1].to_string())
        .to_string();

    // Replace all property keys directly preceded by a `{` or a `,` and followed by a `:` with
    // double-quoted versions.
    map = UNQUOTED_FIELD_RE
        .replace_all(&map, |caps: &regex::Captures| {
            format!("{}\"{}\":", &caps[1], &caps[2])
        })
        .to_string();

    // It *should* be valid JSON now, so parse it with serde_json.
    let parsed: Vec<JsResourceEntry> = serde_json::from_str(&map).unwrap();

    parsed
        .into_iter()
        .filter_map(|(name, props)| {
            // Ignore resources with params for now, since there's no support for them currently.
            if props.params.is_some() {
                None
            } else {
                Some(ResourceProperties {
                    name,
                    alias: props.alias.map(|a| a.to_vec()).unwrap_or_default(),
                    data: props.data,
                })
            }
        })
        .collect()
}

/// Reads data from a file in the form of uBlock Origin's `scriptlets.js` file and produces
/// templatable scriptlets for use in cosmetic filtering.
fn read_template_resources(scriptlets_data: &str) -> Vec<Resource> {
    let mut resources = Vec::new();

    let uncommented = TOP_COMMENT_RE.replace_all(&scriptlets_data, "");
    let mut name: Option<&str> = None;
    let mut details = std::collections::HashMap::<_, Vec<_>>::new();
    let mut script = String::new();

    for line in uncommented.lines() {
        if line.starts_with('#') || line.starts_with("// ") || line == "//" {
            continue;
        }

        if name.is_none() {
            if let Some(stripped) = line.strip_prefix("/// ") {
                name = Some(stripped.trim());
            }
            continue;
        }

        if let Some(stripped) = line.strip_prefix("/// ") {
            let mut line = stripped.split_whitespace();
            let prop = line.next().expect("Detail line has property name");
            let value = line.next().expect("Detail line has property value");
            details
                .entry(prop)
                .and_modify(|v| v.push(value))
                .or_insert_with(|| vec![value]);
            continue;
        }

        if NON_EMPTY_LINE_RE.is_match(line) {
            script += line.trim();
            script.push('\n');
            continue;
        }

        let kind = if script.contains("{{1}}") {
            ResourceType::Template
        } else {
            ResourceType::Mime(MimeType::ApplicationJavascript)
        };

        resources.push(Resource {
            name: name.expect("Resource name must be specified").to_owned(),
            aliases: details
                .get("alias")
                .map(|aliases| aliases.iter().map(|alias| alias.to_string()).collect())
                .unwrap_or_default(),
            kind,
            content: base64::encode(&script),
            dependencies: vec![],
            permission: Default::default(),
        });

        name = None;
        details.clear();
        script.clear();
    }

    resources
}

/// Reads byte data from an arbitrary resource file, and assembles a `Resource` from it with the
/// provided `resource_info`.
fn build_resource_from_file_contents(
    resource_contents: &[u8],
    resource_info: &ResourceProperties,
) -> Resource {
    let name = resource_info.name.to_owned();
    let aliases = resource_info
        .alias
        .iter()
        .map(|alias| alias.to_string())
        .collect();
    let mimetype = MimeType::from_extension(&resource_info.name[..]);
    let content = match mimetype {
        MimeType::ApplicationJavascript | MimeType::TextHtml | MimeType::TextPlain => {
            let utf8string = std::str::from_utf8(resource_contents).unwrap();
            base64::encode(&utf8string.replace('\r', ""))
        }
        _ => base64::encode(&resource_contents),
    };

    Resource {
        name,
        aliases,
        kind: ResourceType::Mime(mimetype),
        content,
        dependencies: vec![],
        permission: Default::default(),
    }
}

/// Produces a `Resource` from the `web_accessible_resource_dir` directory according to the
/// information in `resource_info.
fn read_resource_from_web_accessible_dir(
    web_accessible_resource_dir: &Path,
    resource_info: &ResourceProperties,
) -> Resource {
    let resource_path = web_accessible_resource_dir.join(&resource_info.name);
    if !resource_path.is_file() {
        panic!("Expected {:?} to be a file", resource_path);
    }
    let mut resource_file = File::open(resource_path).expect("open resource file for reading");
    let mut resource_contents = Vec::new();
    resource_file
        .read_to_end(&mut resource_contents)
        .expect("read resource file contents");

    build_resource_from_file_contents(&resource_contents, resource_info)
}

/// Builds a `Vec` of `Resource`s from the specified paths on the filesystem:
///
/// - `web_accessible_resource_dir`: A folder full of resource files
///
/// - `redirect_resources_path`: A file in the format of uBlock Origin's `redirect-resources.js`
/// containing an index of the resources in `web_accessible_resource_dir`
///
/// The resulting resources can be serialized into JSON using `serde_json`.
pub fn assemble_web_accessible_resources(
    web_accessible_resource_dir: &Path,
    redirect_resources_path: &Path,
) -> Vec<Resource> {
    let mapfile_data = std::fs::read_to_string(redirect_resources_path).expect("read aliases path");
    let resource_properties = read_redirectable_resource_mapping(&mapfile_data);

    resource_properties
        .iter()
        .map(|resource_info| {
            read_resource_from_web_accessible_dir(web_accessible_resource_dir, resource_info)
        })
        .collect()
}

/// Parses the _old_ format of uBlock Origin templated scriptlet resources, prior to
/// <https://github.com/gorhill/uBlock/commit/18a84d2819d49444fc31c5350677ecc5b2ec73c6>.
///
/// The newer format is intended to be imported as an ES module, making line-based parsing even
/// more complex and error-prone. Instead, it's recommended to transform them into [Resource]s
/// using JS code. A short prelude containing an array of `[{{1}}, {{2}}, {{3}}, ...]` can be used
/// to backport the newer scriptlet format into the older one; the new one will be directly
/// supported in a future update.
///
/// - `scriptlets_path`: A file in the format of uBlock Origin's `scriptlets.js` containing
/// templatable scriptlet files for use in cosmetic filtering
#[deprecated]
pub fn assemble_scriptlet_resources(scriptlets_path: &Path) -> Vec<Resource> {
    let scriptlets_data = std::fs::read_to_string(scriptlets_path).expect("read scriptlets path");
    read_template_resources(&scriptlets_data)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_war_resource_assembly() {
        let web_accessible_resource_dir =
            Path::new("data/test/fake-uBO-files/web_accessible_resources");
        let redirect_resources_path = Path::new("data/test/fake-uBO-files/redirect-resources.js");
        let resources =
            assemble_web_accessible_resources(web_accessible_resource_dir, redirect_resources_path);

        let expected_resource_names = vec![
            "1x1.gif",
            "2x2.png",
            "3x2.png",
            "32x32.png",
            "addthis_widget.js",
            "amazon_ads.js",
            "amazon_apstag.js",
            "ampproject_v0.js",
            "chartbeat.js",
            //"click-to-load.html" is ignored because it has a params field.
            "doubleclick_instream_ad_status.js",
            "empty",
            "fingerprint2.js",
            "fingerprint3.js",
            "google-analytics_analytics.js",
            "google-analytics_cx_api.js",
            "google-analytics_ga.js",
            "google-analytics_inpage_linkid.js",
            "google-ima.js",
            "googlesyndication_adsbygoogle.js",
            "googletagservices_gpt.js",
            "hd-main.js",
            "ligatus_angular-tag.js",
            "mxpnl_mixpanel.js",
            "monkeybroker.js",
            "noeval.js",
            "noeval-silent.js",
            "nobab.js",
            "nobab2.js",
            "nofab.js",
            "noop-0.1s.mp3",
            "noop-0.5s.mp3",
            "noop-1s.mp4",
            "noop.html",
            "noop.js",
            "noop.txt",
            "noop-vmap1.0.xml",
            "outbrain-widget.js",
            "popads.js",
            "popads-dummy.js",
            "prebid-ads.js",
            "scorecardresearch_beacon.js",
            "window.open-defuser.js",
        ];

        for name in expected_resource_names {
            dbg!(&name);
            assert!(
                resources
                    .iter()
                    .find(|resource| {
                        if let ResourceType::Mime(_) = resource.kind {
                            resource.name == name
                        } else {
                            false
                        }
                    })
                    .is_some(),
                "{:?}",
                name
            );
        }

        let serialized = serde_json::to_string(&resources).expect("serialize resources");

        let reserialized: Vec<Resource> =
            serde_json::from_str(&serialized).expect("deserialize resources");

        assert_eq!(reserialized[0].name, "1x1.gif");
        assert_eq!(reserialized[0].aliases, vec!["1x1-transparent.gif"]);
        assert_eq!(reserialized[0].kind, ResourceType::Mime(MimeType::ImageGif));

        assert_eq!(reserialized[34].name, "noop.js");
        assert_eq!(
            reserialized[34].aliases,
            vec!["noopjs", "abp-resource:blank-js"]
        );
        assert_eq!(
            reserialized[34].kind,
            ResourceType::Mime(MimeType::ApplicationJavascript)
        );
        let noopjs_contents = std::fs::read_to_string(Path::new(
            "data/test/fake-uBO-files/web_accessible_resources/noop.js",
        ))
        .unwrap()
        .replace('\r', "");
        assert_eq!(
            std::str::from_utf8(
                &base64::decode(&reserialized[34].content).expect("decode base64 content")
            )
            .expect("convert to utf8 string"),
            noopjs_contents,
        );
    }

    #[test]
    fn test_scriptlet_resource_assembly2() {
        let scriptlets_path = Path::new("data/test/fake-uBO-files/scriptlets2.js");
        #[allow(deprecated)]
        let resources = assemble_scriptlet_resources(scriptlets_path);

        let expected_resource_names = vec![
            "abort-current-inline-script.js",
            "abort-on-property-read.js",
            "abort-on-property-write.js",
            "abort-on-stack-trace.js",
            "addEventListener-defuser.js",
            "addEventListener-logger.js",
            "json-prune.js",
            "nano-setInterval-booster.js",
            "nano-setTimeout-booster.js",
            "noeval-if.js",
            "no-fetch-if.js",
            "no-floc.js",
            "remove-attr.js",
            "remove-class.js",
            "no-requestAnimationFrame-if.js",
            "set-constant.js",
            "no-setInterval-if.js",
            "no-setTimeout-if.js",
            "webrtc-if.js",
            "window.name-defuser",
            "overlay-buster.js",
            "alert-buster.js",
            "gpt-defuser.js",
            "nowebrtc.js",
            "golem.de.js",
            "upmanager-defuser.js",
            "smartadserver.com.js",
            "adfly-defuser.js",
            "disable-newtab-links.js",
            "damoh-defuser.js",
            "twitch-videoad.js",
            "fingerprint2.js",
            "cookie-remover.js",
        ];

        for name in expected_resource_names {
            assert!(
                resources
                    .iter()
                    .find(|resource| {
                        match resource.kind {
                            ResourceType::Template
                            | ResourceType::Mime(MimeType::ApplicationJavascript) => {
                                resource.name == name
                            }
                            _ => false,
                        }
                    })
                    .is_some(),
                "failed to find {}",
                name
            );
        }

        let serialized = serde_json::to_string(&resources).expect("serialize resources");

        let reserialized: Vec<Resource> =
            serde_json::from_str(&serialized).expect("deserialize resources");

        assert_eq!(reserialized[0].name, "abort-current-inline-script.js");
        assert_eq!(reserialized[0].aliases, vec!["acis.js"]);
        assert_eq!(reserialized[0].kind, ResourceType::Template);

        assert_eq!(reserialized[17].name, "no-setTimeout-if.js");
        assert_eq!(
            reserialized[17].aliases,
            vec!["nostif.js", "setTimeout-defuser.js"]
        );
        assert_eq!(reserialized[17].kind, ResourceType::Template);

        assert_eq!(reserialized[20].name, "overlay-buster.js");
        assert_eq!(reserialized[20].aliases, Vec::<String>::new());
        assert_eq!(
            reserialized[20].kind,
            ResourceType::Mime(MimeType::ApplicationJavascript)
        );
        assert_eq!(
            std::str::from_utf8(
                &base64::decode(&reserialized[20].content).expect("decode base64 content")
            ).expect("convert to utf8 string"),
            "(function() {\nif ( window !== window.top ) {\nreturn;\n}\nvar tstart;\nvar ttl = 30000;\nvar delay = 0;\nvar delayStep = 50;\nvar buster = function() {\nvar docEl = document.documentElement,\nbodyEl = document.body,\nvw = Math.min(docEl.clientWidth, window.innerWidth),\nvh = Math.min(docEl.clientHeight, window.innerHeight),\ntol = Math.min(vw, vh) * 0.05,\nel = document.elementFromPoint(vw/2, vh/2),\nstyle, rect;\nfor (;;) {\nif ( el === null || el.parentNode === null || el === bodyEl ) {\nbreak;\n}\nstyle = window.getComputedStyle(el);\nif ( parseInt(style.zIndex, 10) >= 1000 || style.position === 'fixed' ) {\nrect = el.getBoundingClientRect();\nif ( rect.left <= tol && rect.top <= tol && (vw - rect.right) <= tol && (vh - rect.bottom) < tol ) {\nel.parentNode.removeChild(el);\ntstart = Date.now();\nel = document.elementFromPoint(vw/2, vh/2);\nbodyEl.style.setProperty('overflow', 'auto', 'important');\ndocEl.style.setProperty('overflow', 'auto', 'important');\ncontinue;\n}\n}\nel = el.parentNode;\n}\nif ( (Date.now() - tstart) < ttl ) {\ndelay = Math.min(delay + delayStep, 1000);\nsetTimeout(buster, delay);\n}\n};\nvar domReady = function(ev) {\nif ( ev ) {\ndocument.removeEventListener(ev.type, domReady);\n}\ntstart = Date.now();\nsetTimeout(buster, delay);\n};\nif ( document.readyState === 'loading' ) {\ndocument.addEventListener('DOMContentLoaded', domReady);\n} else {\ndomReady();\n}\n})();\n",
        );

        assert_eq!(reserialized[6].name, "json-prune.js");
        assert_eq!(reserialized[6].aliases, Vec::<String>::new());
        assert_eq!(reserialized[6].kind, ResourceType::Template);
        assert_eq!(
            std::str::from_utf8(
                &base64::decode(&reserialized[6].content).expect("decode base64 content")
            ).expect("convert to utf8 string"),
            "(function() {\nconst rawPrunePaths = '{{1}}';\nconst rawNeedlePaths = '{{2}}';\nconst prunePaths = rawPrunePaths !== '{{1}}' && rawPrunePaths !== ''\n? rawPrunePaths.split(/ +/)\n: [];\nlet needlePaths;\nlet log, reLogNeedle;\nif ( prunePaths.length !== 0 ) {\nneedlePaths = prunePaths.length !== 0 &&\nrawNeedlePaths !== '{{2}}' && rawNeedlePaths !== ''\n? rawNeedlePaths.split(/ +/)\n: [];\n} else {\nlog = console.log.bind(console);\nlet needle;\nif ( rawNeedlePaths === '' || rawNeedlePaths === '{{2}}' ) {\nneedle = '.?';\n} else if ( rawNeedlePaths.charAt(0) === '/' && rawNeedlePaths.slice(-1) === '/' ) {\nneedle = rawNeedlePaths.slice(1, -1);\n} else {\nneedle = rawNeedlePaths.replace(/[.*+?^${}()|[\\]\\\\]/g, '\\\\$&');\n}\nreLogNeedle = new RegExp(needle);\n}\nconst findOwner = function(root, path, prune = false) {\nlet owner = root;\nlet chain = path;\nfor (;;) {\nif ( typeof owner !== 'object' || owner === null  ) {\nreturn false;\n}\nconst pos = chain.indexOf('.');\nif ( pos === -1 ) {\nif ( prune === false ) {\nreturn owner.hasOwnProperty(chain);\n}\nif ( chain === '*' ) {\nfor ( const key in owner ) {\nif ( owner.hasOwnProperty(key) === false ) { continue; }\ndelete owner[key];\n}\n} else if ( owner.hasOwnProperty(chain) ) {\ndelete owner[chain];\n}\nreturn true;\n}\nconst prop = chain.slice(0, pos);\nif (\nprop === '[]' && Array.isArray(owner) ||\nprop === '*' && owner instanceof Object\n) {\nconst next = chain.slice(pos + 1);\nlet found = false;\nfor ( const key of Object.keys(owner) ) {\nfound = findOwner(owner[key], next, prune) || found;\n}\nreturn found;\n}\nif ( owner.hasOwnProperty(prop) === false ) { return false; }\nowner = owner[prop];\nchain = chain.slice(pos + 1);\n}\n};\nconst mustProcess = function(root) {\nfor ( const needlePath of needlePaths ) {\nif ( findOwner(root, needlePath) === false ) {\nreturn false;\n}\n}\nreturn true;\n};\nconst pruner = function(o) {\nif ( log !== undefined ) {\nconst json = JSON.stringify(o, null, 2);\nif ( reLogNeedle.test(json) ) {\nlog('uBO:', location.hostname, json);\n}\nreturn o;\n}\nif ( mustProcess(o) === false ) { return o; }\nfor ( const path of prunePaths ) {\nfindOwner(o, path, true);\n}\nreturn o;\n};\nJSON.parse = new Proxy(JSON.parse, {\napply: function() {\nreturn pruner(Reflect.apply(...arguments));\n},\n});\nResponse.prototype.json = new Proxy(Response.prototype.json, {\napply: function() {\nreturn Reflect.apply(...arguments).then(o => pruner(o));\n},\n});\n})();\n",
        );
    }

    #[test]
    fn test_scriptlet_resource_assembly() {
        let scriptlets_path = Path::new("data/test/fake-uBO-files/scriptlets.js");
        #[allow(deprecated)]
        let resources = assemble_scriptlet_resources(scriptlets_path);

        let expected_resource_names = vec![
            "abort-current-inline-script.js",
            "abort-on-property-read.js",
            "abort-on-property-write.js",
            "addEventListener-defuser.js",
            "addEventListener-logger.js",
            "json-prune.js",
            "nano-setInterval-booster.js",
            "nano-setTimeout-booster.js",
            "noeval-if.js",
            "remove-attr.js",
            "requestAnimationFrame-if.js",
            "set-constant.js",
            "setInterval-defuser.js",
            "no-setInterval-if.js",
            "setTimeout-defuser.js",
            "no-setTimeout-if.js",
            "webrtc-if.js",
            "window.name-defuser",
            "overlay-buster.js",
            "alert-buster.js",
            "gpt-defuser.js",
            "nowebrtc.js",
            "golem.de.js",
            "upmanager-defuser.js",
            "smartadserver.com.js",
            "adfly-defuser.js",
            "disable-newtab-links.js",
            "damoh-defuser.js",
            "twitch-videoad.js",
            "fingerprint2.js",
            "cookie-remover.js",
        ];

        for name in expected_resource_names {
            assert!(
                resources
                    .iter()
                    .find(|resource| {
                        match resource.kind {
                            ResourceType::Template
                            | ResourceType::Mime(MimeType::ApplicationJavascript) => {
                                resource.name == name
                            }
                            _ => false,
                        }
                    })
                    .is_some(),
                "failed to find {}",
                name
            );
        }

        let serialized = serde_json::to_string(&resources).expect("serialize resources");

        let reserialized: Vec<Resource> =
            serde_json::from_str(&serialized).expect("deserialize resources");

        assert_eq!(reserialized[0].name, "abort-current-inline-script.js");
        assert_eq!(reserialized[0].aliases, vec!["acis.js"]);
        assert_eq!(reserialized[0].kind, ResourceType::Template);

        assert_eq!(reserialized[18].name, "overlay-buster.js");
        assert_eq!(reserialized[18].aliases, Vec::<String>::new());
        assert_eq!(
            reserialized[18].kind,
            ResourceType::Mime(MimeType::ApplicationJavascript)
        );
        assert_eq!(
            std::str::from_utf8(
                &base64::decode(&reserialized[18].content).expect("decode base64 content")
            ).expect("convert to utf8 string"),
            "(function() {\nif ( window !== window.top ) {\nreturn;\n}\nvar tstart;\nvar ttl = 30000;\nvar delay = 0;\nvar delayStep = 50;\nvar buster = function() {\nvar docEl = document.documentElement,\nbodyEl = document.body,\nvw = Math.min(docEl.clientWidth, window.innerWidth),\nvh = Math.min(docEl.clientHeight, window.innerHeight),\ntol = Math.min(vw, vh) * 0.05,\nel = document.elementFromPoint(vw/2, vh/2),\nstyle, rect;\nfor (;;) {\nif ( el === null || el.parentNode === null || el === bodyEl ) {\nbreak;\n}\nstyle = window.getComputedStyle(el);\nif ( parseInt(style.zIndex, 10) >= 1000 || style.position === 'fixed' ) {\nrect = el.getBoundingClientRect();\nif ( rect.left <= tol && rect.top <= tol && (vw - rect.right) <= tol && (vh - rect.bottom) < tol ) {\nel.parentNode.removeChild(el);\ntstart = Date.now();\nel = document.elementFromPoint(vw/2, vh/2);\nbodyEl.style.setProperty('overflow', 'auto', 'important');\ndocEl.style.setProperty('overflow', 'auto', 'important');\ncontinue;\n}\n}\nel = el.parentNode;\n}\nif ( (Date.now() - tstart) < ttl ) {\ndelay = Math.min(delay + delayStep, 1000);\nsetTimeout(buster, delay);\n}\n};\nvar domReady = function(ev) {\nif ( ev ) {\ndocument.removeEventListener(ev.type, domReady);\n}\ntstart = Date.now();\nsetTimeout(buster, delay);\n};\nif ( document.readyState === 'loading' ) {\ndocument.addEventListener('DOMContentLoaded', domReady);\n} else {\ndomReady();\n}\n})();\n",
        );
    }
}
