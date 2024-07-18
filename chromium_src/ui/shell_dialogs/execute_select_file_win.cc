/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Can be removed once
// https://chromium-review.googlesource.com/c/chromium/src/+/5711152 arrives in
// Brave.
#define BRAVE_EXECUTE_SELECT_SINGLE_FILE                                 \
  return RunOpenFileDialog(owner, title, std::u16string(), default_path, \
                           filter, 0, filter_index, paths);

// Can be removed once
// https://chromium-review.googlesource.com/c/chromium/src/+/5711152 arrives in
// Brave.
#define BRAVE_EXECUTE_SELECT_MULTIPLE_FILE                               \
  return RunOpenFileDialog(owner, title, std::u16string(), default_path, \
                           filter, dialog_options, filter_index, paths);

// Can be removed once
// https://chromium-review.googlesource.com/c/chromium/src/+/5711152 arrives in
// Brave.
#define BRAVE_EXECUTE_SAVE_FILE                                                \
  return RunSaveFileDialog(owner, title, default_path, filter, dialog_options, \
                           def_ext, filter_index, path);

#include "src/ui/shell_dialogs/execute_select_file_win.cc"

#undef BRAVE_EXECUTE_SAVE_FILE
#undef BRAVE_EXECUTE_SELECT_MULTIPLE_FILE
#undef BRAVE_EXECUTE_SELECT_SINGLE_FILE
