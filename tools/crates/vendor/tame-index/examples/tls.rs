//! As of `tame-index` 0.26, it's now required that TLS is configured by the user
//! before passing it to `tame-index`, when using the `sparse` feature.
//!
//! This example illustrates how to configure TLS using `rustls` as the backend,
//! `ring` as the crypto provider, and `webpki-roots` as the certificate roots.
//!
//! Requirements as a user:
//!
//! 1. Set `default-features = false` on `reqwest` to disable the automatic use of rustls + aws-lc-rs
//! 2. Add the `rustls-no-provider` feature to `reqwest` to enable the `tls_backend_preconfigured` method
//! 3. Add dependencies on `rustls` (the same version `reqwest` uses) and `webpki-roots`

#[cfg(not(feature = "sparse"))]
fn main() {}

#[cfg(feature = "sparse")]
fn main() {
    // Create a certificate store using webpki_roots, which packages
    let rcs: rustls::RootCertStore = webpki_roots::TLS_SERVER_ROOTS.iter().cloned().collect();
    let client_config = rustls::ClientConfig::builder_with_provider(std::sync::Arc::new(
        // Use `ring` as the crypto provider
        rustls::crypto::ring::default_provider(),
    ))
    .with_protocol_versions(rustls::DEFAULT_VERSIONS)
    .unwrap()
    .with_root_certificates(rcs)
    .with_no_client_auth();

    let client = reqwest::blocking::Client::builder()
        // Set the TLS backend. Note that this *requires* that the version of
        // rustls is the same as the one reqwest is using
        .tls_backend_preconfigured(client_config)
        .build()
        .expect("failed to build client");

    let td = tempfile::TempDir::new_in("target/tmp").unwrap();
    let index = tame_index::SparseIndex::new(
        tame_index::IndexLocation::new(tame_index::IndexUrl::CratesIoSparse).with_root(Some(
            tame_index::PathBuf::from_path_buf(td.path().to_owned()).unwrap(),
        )),
    )
    .unwrap();
    // We're using a unique temp directory
    let lock = tame_index::utils::flock::FileLock::unlocked();

    let rsi = tame_index::index::RemoteSparseIndex::new(index, client);

    // Fetch the metadata for the spdx crate
    let spdx_krate = rsi
        .krate("spdx".try_into().unwrap(), true, &lock)
        .expect("failed to retrieve spdx")
        .expect("failed to find spdx");

    // Print out the semver for each version in the index
    for v in spdx_krate.versions {
        println!("{}", v.version);
    }
}
