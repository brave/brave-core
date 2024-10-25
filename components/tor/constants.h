/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_CONSTANTS_H_
#define BRAVE_COMPONENTS_TOR_CONSTANTS_H_

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/safe_base_name.h"
#include "build/build_config.h"

namespace tor {

#if BUILDFLAG(IS_WIN)
inline constexpr char kTorClientComponentName[] =
    "Brave Tor Client Updater (Windows)";
inline constexpr char kTorClientComponentId[] =
    "cpoalefficncklhjfpglfiplenlpccdb";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1AYAsmR/VoRwkZCsjRpD"
    "58xjrgngW5y17H6BqQ7/CeNSpmXlcMXy6bJs2D/yeS96rhZSrQSHTzS9h/ieo/NZ"
    "F5PIwcv07YsG5sRd6zF5a6m92aWCQa1OkbL6jpcpL2Tbc4mCqNxhKMErT7EtIIWL"
    "9cW+mtFUjUjvV3rJLQ3Vy9u6fEi77Y8b25kGnTJoVt3uETAIHBnyNpL7ac2f8Iq+"
    "4Qa6VFmuoBhup54tTZvMv+ikoKKaQkHzkkjTa4hV5AzdnFDKO8C9qJb3T/Ef0+MO"
    "IuZjyySVzGNcOfASeHkhxhlwMQSQuhCN5mdFW5YBnVZ/5QWx8WzbhqBny/ZynS4e"
    "rQIDAQAB";
#elif BUILDFLAG(IS_MAC)
inline constexpr char kTorClientComponentName[] =
    "Brave Tor Client Updater (Mac)";
inline constexpr char kTorClientComponentId[] =
    "cldoidikboihgcjfkhdeidbpclkineef";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAw2QUXSbVuRxYpItYApZ8"
    "Ly/fGeUD3A+vb3J7Ot62CF32wTfWweANWyyB+EBGfbtNDAuRlAbNk0QYeCQEttuf"
    "jLh3Kd5KR5fSyyNNd2cAzAckQ8p7JdiFYjvqZLGC5vlnHgqq4O8xACX5EPwHLNFD"
    "iSpsthNmz3GCUrHrzPHjHVfy+IuucQXygnRv2fwIaAIxJmTbYm4fqsGKpfolWdMe"
    "jKVAy1hc9mApZSyt4oGvUu4SJZnxlYMrY4Ze+OWbDesi2JGy+6dA1ddL9IdnwCb3"
    "9CBOMNjaHeCVz0MKxdCWGPieQM0R7S1KvDCVqAkss6NAbLB6AVM0JulqxC9b+hr/"
    "xwIDAQAB";
#elif BUILDFLAG(IS_LINUX)
inline constexpr char kTorClientComponentName[] =
    "Brave Tor Client Updater (Linux)";
#if defined(ARCH_CPU_ARM64)
inline constexpr char kTorClientComponentId[] =
    "monolafkoghdlanndjfeebmdfkbklejg";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzqb14fggDpbjZtv3HKmR"
    "UTnvfDTcqVbVZo0DdCHQi6SwxDlRweGwsvsHuy9U37VBr41ha/neemQGf+5qkWgY"
    "y+mzzAkb5ZtrHkBSOOsZdyO9WEj7GwXuAx9FvcxG2zPpA/CvagnC14VhMyUFLL8v"
    "XdfHYPmQOtIVdW3eR0G/4JP/mTbnAEkipQfxrDMtDVpX+FDB+Zy5yEMGKWHRLcdH"
    "bHUgb/VhB9ppt0LKRjM44KSpyPDlYquXNcn3WFmxHoVm7PZ3LTAn3eSNZrT4ptmo"
    "KveT4LgWtObrHoZtrg+/LnHAi1GYf8PHrRc+o/FptobOWoUN5lt8NvhLjv85ERBt"
    "rQIDAQAB";
#else  // #if defined(ARCH_CPU_ARM64)
inline constexpr char kTorClientComponentId[] =
    "biahpgbdmdkfgndcmfiipgcebobojjkp";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAseuq8dXKawkZC7RSE7xb"
    "lRwh6DD+oPEGEjZWKh596/42IrWNQw60gRIR6s7x0YHh5geFnBRkx9bisEXOrFkq"
    "oArVY7eD0gMkjpor9CneD5CnCxc9/2uIPajtXfAmmLAHtN6Wk7yW30SkRf/WvLWX"
    "/H+PqskQBN7I5MO7sveYxSrRMSj7prrFHEiFmXTgG/DwjpzrA7KV6vmzz/ReD51o"
    "+UuLHE7cxPhnsNd/52uY3Lod3GhxvDoXKYx9kWlzBjxB53A2eLBCDIwwCpqS4/Ib"
    "RSJhvF33KQT8YM+7V1MitwB49klP4aEWPXwOlFHmn9Dkmlx2RbO7S0tRcH9UH4LK"
    "2QIDAQAB";
#endif
#endif

// Returns the path for for where the Tor client binary is installed.
base::FilePath GetTorClientDirectory();

// Returns the path client execution path, based on the installation path for
// components, the `install_dir` provided, and the `filename`.
base::FilePath GetClientExecutablePath(const base::SafeBaseName& install_dir,
                                       const base::SafeBaseName& executable);

// Returns the path for the torrc file, based on the installation path for
// components, and the `install_dir` provided.
base::FilePath GetTorRcPath(const base::SafeBaseName& install_dir);

// Returns the path for the client's `--DataDirectory` argument.
base::FilePath GetTorDataPath();

// Return the directory path for the watcher arguments passed to the client.
base::FilePath GetTorWatchPath();

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_CONSTANTS_H_
