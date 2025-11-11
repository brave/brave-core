// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/download/bubble/download_bubble_row_view.h"

#include "base/files/file_path.h"
#include "chrome/browser/ui/download/download_bubble_row_view_info.h"

namespace {
const base::FilePath kTestFilePath(FILE_PATH_LITERAL("foo/bar.cc"));
}  // namespace

// Provides default implementation for mock download item method in SetUp().
#define SetInputProtectorForTesting(...)    \
  SetInputProtectorForTesting(__VA_ARGS__); \
  ON_CALL(download_item_, GetFullPath()).WillByDefault(ReturnRef(kTestFilePath))

// Override test comparison for quick actions. We append a command to delete
// local file. Check if the last command is the one we expect, and pop back it
// so that it does not affect the rest of the test.
#define quick_actions()                                                 \
  quick_actions().back().command == DownloadCommands::DELETE_LOCAL_FILE \
      ? [&]() {                                                         \
          auto actions = row_view()->info().quick_actions();            \
          actions.pop_back();                                           \
          return actions.size();                                        \
        }()                                                             \
      : row_view()->info().quick_actions()

#include <chrome/browser/ui/views/download/bubble/download_bubble_row_view_unittest.cc>

#undef quick_actions
#undef SetInputProtectorForTesting
