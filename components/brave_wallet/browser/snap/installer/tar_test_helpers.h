/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_TAR_TEST_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_TAR_TEST_HELPERS_H_

#include <string>
#include <utility>
#include <vector>

namespace brave_wallet {

// Builds a raw POSIX ustar archive from (path, content) pairs. Each path must
// be at most 100 bytes (the ustar name field; no prefix splitting is done).
std::string BuildUstarTar(
    const std::vector<std::pair<std::string, std::string>>& entries);

// Builds the uncompressed tar bytes for a snap package containing
// "package/snap.manifest.json" and "package/<bundle_file_path>".
std::string BuildSnapTar(
    const std::string& manifest_json,
    const std::string& bundle_js,
    const std::string& bundle_file_path = "dist/bundle.js");

// gzip-compresses |data|; the install pipeline expects a .tgz on the wire.
std::string GzipCompressForTest(const std::string& data);

// Convenience: builds a gzipped snap tarball from |manifest_json| +
// |bundle_js|. Equivalent to GzipCompressForTest(BuildSnapTar(...)).
std::string BuildSnapTarball(
    const std::string& manifest_json,
    const std::string& bundle_js,
    const std::string& bundle_file_path = "dist/bundle.js");

// Returns base64(sha256(bundle_js)) — the source.shasum expected by the
// tar decompressor for a source-only snap.
std::string ComputeSnapBundleShasum(const std::string& bundle_js);

// Builds a minimal snap.manifest.json string with the bundle path and shasum.
std::string MakeMinimalSnapManifestJson(
    const std::string& bundle_file_path = "dist/bundle.js",
    const std::string& shasum = "test-shasum");

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_INSTALLER_TAR_TEST_HELPERS_H_
