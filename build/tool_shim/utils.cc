/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/build/tool_shim/utils.h"

#include "base/environment.h"
#include "base/logging.h"

base::FilePath ResolveAgainst(const base::FilePath& base_dir,
                              const base::FilePath::StringType& path) {
  const base::FilePath file_path(path);
  const base::FilePath resolved =
      file_path.IsAbsolute() ? file_path : base_dir.Append(file_path);
  return resolved;
}

void ConfigureLogging() {
  auto environment = base::Environment::Create();
  if (environment->GetVar("TOOL_SHIM_VERBOSE").value_or("") != "1") {
    return;
  }

  // Enable VLOG(1) logging.
  logging::SetMinLogLevel(-1);

  // Disable timestamps, process IDs, thread IDs, etc.
  logging::SetLogItems(false, false, false, false);
}

base::FilePath::StringType ToNativeString(std::string_view value) {
  return base::FilePath::FromUTF8Unsafe(value).value();
}
