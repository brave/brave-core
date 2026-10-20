// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_INCOMING_FILE_SINK_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_INCOMING_FILE_SINK_H_

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/functional/callback.h"

namespace brave_transfer {

struct SavedFile {
  base::FilePath path;
  // The file went to the browser download folder because the chosen save
  // folder was unavailable.
  bool used_fallback = false;
};

// Writes received files. Each file is identified by a caller-chosen `key`
// from Begin() until Commit() or Abort().
class IncomingFileSink {
 public:
  using CommitCallback =
      base::OnceCallback<void(std::optional<SavedFile> saved)>;

  virtual ~IncomingFileSink() = default;

  // Starts a file. `name` must already be sanitized. `on_error` runs at most
  // once, if the file cannot be created or written; the file is then
  // discarded and further calls for `key` are ignored.
  virtual void Begin(const std::string& key,
                     const std::string& name,
                     base::OnceClosure on_error) = 0;
  // Appends data in order.
  virtual void Append(const std::string& key, std::string data) = 0;
  // Finishes the file and moves it to its final, unique name. Runs `callback`
  // with nullopt on failure.
  virtual void Commit(const std::string& key, CommitCallback callback) = 0;
  // Deletes the partial file.
  virtual void Abort(const std::string& key) = 0;
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_INCOMING_FILE_SINK_H_
