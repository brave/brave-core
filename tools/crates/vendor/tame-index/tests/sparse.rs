#![allow(missing_docs)]

mod utils;

use http::header;
use tame_index::{IndexLocation, IndexUrl, SparseIndex};

#[inline]
fn crates_io(path: impl AsRef<tame_index::Path>) -> SparseIndex {
    SparseIndex::new(
        IndexLocation::new(IndexUrl::CratesIoSparse).with_root(Some(path.as_ref().to_owned())),
    )
    .unwrap()
}

/// Validates the we get a valid root and krate url
#[test]
fn opens_crates_io() {
    let index = crates_io(env!("CARGO_MANIFEST_DIR"));

    assert_eq!(index.url(), "https://index.crates.io/");
    assert_eq!(
        index.crate_url("autocfg".try_into().unwrap()),
        "https://index.crates.io/au/to/autocfg"
    );
}

/// Validates a request can be made for a crate that doesn't have a local cache entry
#[test]
fn make_request_without_cache() {
    let index = crates_io(env!("CARGO_MANIFEST_DIR"));
    let lock = &utils::unlocked();

    let req = index
        .make_remote_request("serde".try_into().unwrap(), None, lock)
        .unwrap();

    let hdrs = req.headers();
    // Ensure neither of the possible cache headers are set
    assert!(
        hdrs.get(header::IF_MODIFIED_SINCE).is_none() && hdrs.get(header::IF_NONE_MATCH).is_none()
    );

    assert_eq!(hdrs.get("cargo-protocol").unwrap(), "version=1");
    assert_eq!(hdrs.get(header::ACCEPT).unwrap(), "text/plain");
    assert_eq!(hdrs.get(header::ACCEPT_ENCODING).unwrap(), "gzip");
}

const ETAG: &str = "W/\"fa62f662c9aae1f21cab393950d4ae23\"";
const DATE: &str = "Thu, 22 Oct 2023 09:40:03 GMT";

/// Validates appropriate headers are set when a cache entry does exist
#[test]
fn make_request_with_cache() {
    let td = utils::tempdir();
    let index = crates_io(&td);
    let lock = &utils::unlocked();

    {
        let etag_krate = utils::fake_krate("etag-krate", 2);
        index
            .cache()
            .write_to_cache(&etag_krate, &format!("{}: {ETAG}", header::ETAG), lock)
            .unwrap();

        let req = index
            .make_remote_request("etag-krate".try_into().unwrap(), None, lock)
            .unwrap();

        assert_eq!(req.headers().get(header::IF_NONE_MATCH).unwrap(), ETAG);
    }

    {
        let req = index
            .make_remote_request("etag-specified-krate".try_into().unwrap(), Some(ETAG), lock)
            .unwrap();

        assert_eq!(req.headers().get(header::IF_NONE_MATCH).unwrap(), ETAG);
    }

    {
        let modified_krate = utils::fake_krate("modified-krate", 2);
        index
            .cache()
            .write_to_cache(
                &modified_krate,
                &format!("{}: {DATE}", header::LAST_MODIFIED),
                lock,
            )
            .unwrap();

        let req = index
            .make_remote_request("modified-krate".try_into().unwrap(), None, lock)
            .unwrap();

        assert_eq!(req.headers().get(header::IF_MODIFIED_SINCE).unwrap(), DATE);
    }
}

/// Validates we can parse a response where the local cache version is up to date
#[test]
fn parse_unmodified_response() {
    let td = utils::tempdir();
    let index = crates_io(&td);
    let lock = &utils::unlocked();

    let etag_krate = utils::fake_krate("etag-krate", 2);
    index
        .cache()
        .write_to_cache(&etag_krate, &format!("{}: {ETAG}", header::ETAG), lock)
        .unwrap();

    let response = http::Response::builder()
        .status(http::StatusCode::NOT_MODIFIED)
        .header(header::ETAG, ETAG)
        .body(Vec::new())
        .unwrap();

    let cached_krate = index
        .parse_remote_response("etag-krate".try_into().unwrap(), response, true, lock)
        .unwrap()
        .expect("cached krate");

    assert_eq!(etag_krate, cached_krate);
}

/// Validates we can parse a modified response
#[test]
fn parse_modified_response() {
    let td = utils::tempdir();
    let index = crates_io(&td);
    let lock = &utils::unlocked();

    {
        let etag_krate = utils::fake_krate("etag-krate", 3);
        let mut serialized = Vec::new();
        etag_krate.write_json_lines(&mut serialized).unwrap();

        let response = http::Response::builder()
            .status(http::StatusCode::OK)
            .header(header::ETAG, ETAG)
            .body(serialized)
            .unwrap();

        let new_krate = index
            .parse_remote_response("etag-krate".try_into().unwrap(), response, true, lock)
            .unwrap()
            .expect("new response");

        assert_eq!(etag_krate, new_krate);

        let cached_krate = index
            .cache()
            .cached_krate(
                "etag-krate".try_into().unwrap(),
                Some(&format!("{}: {ETAG}", header::ETAG)),
                lock,
            )
            .unwrap()
            .expect("cached krate");

        assert_eq!(etag_krate, cached_krate);
    }

    {
        let modified_krate = utils::fake_krate("modified-krate", 3);
        let mut serialized = Vec::new();
        modified_krate.write_json_lines(&mut serialized).unwrap();

        let response = http::Response::builder()
            .status(http::StatusCode::OK)
            .header(header::LAST_MODIFIED, DATE)
            .body(serialized)
            .unwrap();

        let new_krate = index
            .parse_remote_response("modified-krate".try_into().unwrap(), response, true, lock)
            .unwrap()
            .expect("new response");

        assert_eq!(modified_krate, new_krate);

        let cached_krate = index
            .cache()
            .cached_krate(
                "modified-krate".try_into().unwrap(),
                Some(&format!("{}: {DATE}", header::LAST_MODIFIED)),
                lock,
            )
            .unwrap()
            .expect("cached krate");

        assert_eq!(modified_krate, cached_krate);
    }
}

#[cfg(feature = "sparse")]
mod remote {
    use super::*;

    /// Ensure we can actually send a request to crates.io and parse the response
    #[test]
    fn end_to_end() {
        let td = utils::tempdir();
        let index = crates_io(&td);
        let lock = &utils::unlocked();

        let client = reqwest::blocking::Client::builder().build().unwrap();

        let rsi = tame_index::index::RemoteSparseIndex::new(index, client);

        let spdx_krate = rsi
            .krate("spdx".try_into().unwrap(), true, lock)
            .expect("failed to retrieve spdx")
            .expect("failed to find spdx");

        spdx_krate
            .versions
            .iter()
            .find(|iv| iv.version == "0.10.1")
            .expect("failed to find expected version");
    }

    // cargo metadata --format-version=1 | jq -r '.packages[].name | "\"\(.)\","'
    const KRATES: &[&str] = &[
        "addr2line",
        "adler",
        "async-compression",
        "autocfg",
        "backtrace",
        "base64",
        "bitflags",
        "bitflags",
        "bumpalo",
        "bytes",
        "camino",
        "cargo-platform",
        "cargo_metadata",
        "cc",
        "cfg-if",
        "core-foundation",
        "core-foundation-sys",
        "crc32fast",
        "crossbeam-deque",
        "crossbeam-epoch",
        "crossbeam-utils",
        "either",
        "encoding_rs",
        "equivalent",
        "errno",
        "fastrand",
        "flate2",
        "fnv",
        "form_urlencoded",
        "futures-channel",
        "futures-core",
        "futures-io",
        "futures-sink",
        "futures-task",
        "futures-util",
        "getrandom",
        "gimli",
        "h2",
        "hashbrown",
        "hermit-abi",
        "home",
        "http",
        "http-body",
        "httparse",
        "httpdate",
        "hyper",
        "hyper-rustls",
        "idna",
        "indexmap",
        "ipnet",
        "itoa",
        "js-sys",
        "libc",
        "linux-raw-sys",
        "log",
        "memchr",
        "mime",
        "miniz_oxide",
        "mio",
        "num_cpus",
        "object",
        "once_cell",
        "percent-encoding",
        "pin-project-lite",
        "pin-utils",
        "proc-macro2",
        "quote",
        "rayon",
        "rayon-core",
        "reqwest",
        "ring",
        "rustc-demangle",
        "rustix",
        "rustls",
        "rustls-pemfile",
        "rustls-webpki",
        "ryu",
        "sct",
        "semver",
        "serde",
        "serde_derive",
        "serde_json",
        "serde_spanned",
        "serde_urlencoded",
        "slab",
        "smol_str",
        "socket2",
        "spin",
        "static_assertions",
        "syn",
        "sync_wrapper",
        "system-configuration",
        "system-configuration-sys",
        "tame-index",
        "tempfile",
        "thiserror",
        "thiserror-impl",
        "tiny-bench",
        "tinyvec",
        "tinyvec_macros",
        "tokio",
        "tokio-rustls",
        "tokio-util",
        "toml",
        "toml_datetime",
        "toml_edit",
        "tower-service",
        "tracing",
        "tracing-core",
        "try-lock",
        "twox-hash",
        "unicode-bidi",
        "unicode-ident",
        "unicode-normalization",
        "untrusted",
        "url",
        "want",
        "wasi",
        "wasm-bindgen",
        "wasm-bindgen-backend",
        "wasm-bindgen-futures",
        "wasm-bindgen-macro",
        "wasm-bindgen-macro-support",
        "wasm-bindgen-shared",
        "web-sys",
        "webpki-roots",
        "windows-sys",
        "windows-sys",
        "windows-targets",
        "windows-targets",
        "windows_aarch64_gnullvm",
        "windows_aarch64_gnullvm",
        "windows_aarch64_msvc",
        "windows_aarch64_msvc",
        "windows_i686_gnu",
        "windows_i686_gnu",
        "windows_i686_msvc",
        "windows_i686_msvc",
        "windows_x86_64_gnu",
        "windows_x86_64_gnu",
        "windows_x86_64_gnullvm",
        "windows_x86_64_gnullvm",
        "windows_x86_64_msvc",
        "windows_x86_64_msvc",
        "winnow",
        "winreg",
    ];

    fn ensure_no_errors(
        results: std::collections::BTreeMap<
            String,
            Result<Option<tame_index::IndexKrate>, tame_index::Error>,
        >,
    ) {
        use std::fmt::Write;
        let mut errors = String::new();

        for (name, res) in results {
            match res {
                Ok(Some(_)) => {}
                Ok(None) => writeln!(&mut errors, "{name}:\tfailed to locate").unwrap(),
                Err(err) => writeln!(&mut errors, "{name}:\t{err}").unwrap(),
            }
        }

        assert!(errors.is_empty(), "{errors}");
    }

    /// Reuses connections. This test is intended to be run under strace to
    /// validate that <n> connections are not being created
    /// <https://github.com/EmbarkStudios/tame-index/issues/46>
    #[test]
    fn reuses_connection() {
        let td = utils::tempdir();
        let index = crates_io(&td);
        let lock = &utils::unlocked();

        let client = reqwest::blocking::Client::builder().build().unwrap();
        let rsi = tame_index::index::RemoteSparseIndex::new(index, client);

        let results = rsi.krates(
            KRATES.iter().map(|s| (*s).to_string()).collect(),
            false,
            lock,
        );

        ensure_no_errors(results);
    }

    // Ditto, but for async
    #[test]
    fn async_reuses_connection() {
        let rt = tokio::runtime::Runtime::new().unwrap();
        let _guard = rt.enter();

        let td = utils::tempdir();
        let index = crates_io(&td);
        let lock = &utils::unlocked();

        let client = reqwest::Client::builder().build().unwrap();
        let rsi = tame_index::index::AsyncRemoteSparseIndex::new(index, client);

        let results = rsi
            .krates_blocking(
                KRATES.iter().map(|s| (*s).to_string()).collect(),
                false,
                None,
                lock,
            )
            .unwrap();

        ensure_no_errors(results);
    }
}
