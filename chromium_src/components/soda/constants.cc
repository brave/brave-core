/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Override the SODA binary path to point to the Sherpa-ONNX shared library.
// The original SODA binary (libsoda.so) requires a Google API key that is
// unavailable in non-Chrome builds. Sherpa-ONNX is an open-source (Apache 2.0)
// drop-in replacement that requires no API key.

#include "build/build_config.h"

// Rename the original constant and the function that uses it, so we can
// provide replacements that point to the Sherpa-ONNX library.
#define kSodaBinaryRelativePath kSodaBinaryRelativePath_ChromiumImpl
#define GetSodaBinaryPath GetSodaBinaryPath_ChromiumImpl

#include <components/soda/constants.cc>

#undef GetSodaBinaryPath
#undef kSodaBinaryRelativePath

namespace speech {

// Provide the extern-declared kSodaBinaryRelativePath with our path.
extern const base::FilePath::CharType kSodaBinaryRelativePath[];
#if BUILDFLAG(IS_WIN)
const base::FilePath::CharType kSodaBinaryRelativePath[] =
    FILE_PATH_LITERAL("SODAFiles/sherpa-onnx-c-api.dll");
#elif BUILDFLAG(IS_MAC)
const base::FilePath::CharType kSodaBinaryRelativePath[] =
    FILE_PATH_LITERAL("SODAFiles/libsherpa-onnx-c-api.dylib");
#else
const base::FilePath::CharType kSodaBinaryRelativePath[] =
    FILE_PATH_LITERAL("SODAFiles/libsherpa-onnx-c-api.so");
#endif  // BUILDFLAG(IS_WIN)

const base::FilePath GetSodaBinaryPath() {
  base::FilePath soda_dir = GetLatestSodaDirectory();
  return soda_dir.empty() ? base::FilePath()
                          : soda_dir.Append(kSodaBinaryRelativePath);
}

}  // namespace speech
