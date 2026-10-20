// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_DIRECTORY_FILE_SINK_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_DIRECTORY_FILE_SINK_H_

#include <map>
#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_transfer/browser/incoming_file_sink.h"

namespace brave_transfer {

// Writes received files into a folder on a blocking sequence: first to
// "<name>.part", then renamed to a unique "<name>" on Commit().
class DirectoryFileSink : public IncomingFileSink {
 public:
  struct Directories {
    // Where files should go. Empty means `fallback`.
    base::FilePath preferred;
    // Used when `preferred` is missing or not writable.
    base::FilePath fallback;
  };
  // Called on the owning sequence for every file, so settings changes apply
  // to the next file received.
  using DirectoriesCallback = base::RepeatingCallback<Directories()>;
  // Runs on the file sequence after a file gets its final name, for example
  // to quarantine it. May be null.
  using FinalizeCallback =
      base::RepeatingCallback<void(const base::FilePath& path)>;

  DirectoryFileSink(
      DirectoriesCallback directories,
      FinalizeCallback finalize,
      scoped_refptr<base::SequencedTaskRunner> file_task_runner = nullptr);
  DirectoryFileSink(const DirectoryFileSink&) = delete;
  DirectoryFileSink& operator=(const DirectoryFileSink&) = delete;
  ~DirectoryFileSink() override;

  // IncomingFileSink:
  void Begin(const std::string& key,
             const std::string& name,
             base::OnceClosure on_error) override;
  void Append(const std::string& key, std::string data) override;
  void Commit(const std::string& key, CommitCallback callback) override;
  void Abort(const std::string& key) override;

 private:
  class Writer {
   public:
    explicit Writer(FinalizeCallback finalize);
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;
    ~Writer();

    bool Begin(const std::string& key,
               const std::string& name,
               const Directories& directories);
    bool Append(const std::string& key, const std::string& data);
    std::optional<SavedFile> Commit(const std::string& key);
    void Abort(const std::string& key);

   private:
    struct OpenFile {
      OpenFile();
      OpenFile(OpenFile&&);
      OpenFile& operator=(OpenFile&&);
      ~OpenFile();

      base::File file;
      base::FilePath part_path;
      base::FilePath final_path;
      bool used_fallback = false;
    };

    std::optional<OpenFile> Create(const base::FilePath& directory,
                                   const std::string& name);

    FinalizeCallback finalize_;
    std::map<std::string, OpenFile> files_;
  };

  void OnWriteResult(const std::string& key, bool success);
  void OnCommitted(const std::string& key,
                   CommitCallback callback,
                   std::optional<SavedFile> saved);
  void Fail(const std::string& key);

  DirectoriesCallback directories_;
  base::SequenceBound<Writer> writer_;
  base::flat_map<std::string, base::OnceClosure> error_callbacks_;
  base::flat_set<std::string> failed_;
  base::WeakPtrFactory<DirectoryFileSink> weak_factory_{this};
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_DIRECTORY_FILE_SINK_H_
