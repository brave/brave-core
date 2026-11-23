//! # rustls-platform-verifier-android
//!
//! This crate is an implementation detail of the actual [rustls-platform-verifier](https://github.com/rustls/rustls-platform-verifier) crate.
//!
//! It contains no Rust code and is solely intended as a convenient mechanism to synchronize a SemVer version managed by `cargo` to Gradle in
//! Android build systems in such a way that a SemVer incompatible version of the native component is never used.
//!
//! Other crates should not directly depend on this crate in any way, as nothing about it is considered stable and it is probably useless elsewhere.
//!
//! ## Details
//!
//! Note: Everything in this section is subject to change at any time. Semver may not be followed.
//!
//! ### Why?
//!
//! It is the best known middle ground between several tradeoffs. The important ones, in priority order, are:
//! - Automatically keeping component versions in sync
//! - Allowing well-tested and well-known `cargo` dependency management patterns to apply everywhere
//! - Providing a smooth developer experience as an Android consumer of `rustls-platform-verifier`
//!
//! Firstly, what alternatives are available for distributing the component? The other known ones are:
//! - Source distribution in some form (here, it will be through crates.io)
//! - Maven Central (or another hosted package registry)
//! - Bundling Android release artifacts inside crates.io releases
//!
//! Starting with the first, its infeasible due to toolchain syncing requirements. If the Android component is built as part of the host
//! app's Gradle build, then it becomes subject to any Gradle or Android Gradle Plugin incompatibilities/requirements. In practice this means
//! the AGP version between this project and the main application have to match all the time. Sometimes this works, but it becomes challenging/unfeasible
//! during yearly toolchain/SDK upgrades and is not maintainable long term.
//!
//! Next, Maven Central. This is considered the standard way of distributing public Android dependencies. There are two downsides to this
//! approach: version synchronization and publishing overhead. Version syncing is the hardest part: There's not a good way to know what version
//! a crate is that doesn't hurt the Cargo part of the build or damage functionality. So instead of making assumptions at runtime, we would need to do
//! clunky and manual version counting with an extra error case. Less importantly, the admin overhead of Maven Central is non-zero so its good to avoid
//! if possible for such a small need.
//!
//! It is also worth calling out a third set of much worse options: requiring users to manually download and install the Android component
//! on each update, which magnifies the version syncing problem with lots of user overhead and then deleting the component outright. A rewrite
//! could be done with raw JNI calls, but this would easily be 3x the size of the existing implementation and require huge amounts of `unsafe`
//! to review then audit.
//!
//! ### The solution
//!
//! The current design was built after running into several painpoints with the previous attempted distribution implementations and the need to start including
//! more than just Android code in releases. To produce the release, we rely on packaging scripts to build the Android component into a prebuilt AAR file.
//! Next, a [on-disk Maven repository](https://maven.apache.org/repositories/local.html) is hosted inside of this repository with a special branch on GitHub.
//! Using GitHub's ability to serve raw files, this local repository creates an emulated Maven package repository that can be queried and downloaded from like a hosted registry.
//!
//! The remaining parts are filled in during the packaging/release process, with artifacts being pushed from release branches into the special archive branch.
//! The main crate ensures it always uses a compatible version from this local repository by declaring a standard platform-specific dependency on this shim crate.
//! Cargo lockfile resolution takes care of the rest.
//!
//! On [the Gradle side](https://github.com/rustls/rustls-platform-verifier/tree/main#gradle-setup), we instruct users to include a small code snippet in their `settings.gradle` file
//! to dynamically resolve a correct Android library's version to download like any other. When the snippet is run, it finds the version inside the workspace's `Cargo.lock`
//! and provides that to Gradle's version resolution. When the lockfile is changed, the configuration cache is invalidated and the version is calculated again.
//! This happens after any version updates (semver, Git refs, etc).
//!
//! ## Summary
//!
//! In summary, the selected distribution method avoids most of the previous pitfalls while still balancing a good experience for `cargo` and Gradle users. Some of its
//! positive properties include:
//! - Full compatibility with Cargo's dependency management, including Git patching[^1]
//! - No version checking or manual synchronization required
//! - Painless and harmless to integrate into an Android app's build system
//! - Low maintenance for the main crate maintainers'
//!
//! [^1]: The Git reference being used must have an equivalent Maven repository branch inside of it and the Maven repository URL must be switched too.
