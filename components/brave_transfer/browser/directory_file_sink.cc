// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/directory_file_sink.h"

#include <utility>

#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"

namespace brave_transfer {

DirectoryFileSink::Writer::OpenFile::OpenFile() = default;
DirectoryFileSink::Writer::OpenFile::OpenFile(OpenFile&&) = default;
DirectoryFileSink::Writer::OpenFile&
DirectoryFileSink::Writer::OpenFile::operator=(OpenFile&&) = default;
DirectoryFileSink::Writer::OpenFile::~OpenFile() = default;

DirectoryFileSink::Writer::Writer(FinalizeCallback finalize)
    : finalize_(std::move(finalize)) {}

DirectoryFileSink::Writer::~Writer() {
  for (auto& [key, open] : files_) {
    open.file.Close();
    base::DeleteFile(open.part_path);
  }
}

std::optional<DirectoryFileSink::Writer::OpenFile>
DirectoryFileSink::Writer::Create(const base::FilePath& directory,
                                  const std::string& name) {
  if (directory.empty()) {
    return std::nullopt;
  }
  if (!base::DirectoryExists(directory) && !base::CreateDirectory(directory)) {
    return std::nullopt;
  }
  const base::FilePath target =
      directory.Append(base::FilePath::FromUTF8Unsafe(name));
  if (target.DirName() != directory) {
    return std::nullopt;
  }
  const base::FilePath part_path =
      base::GetUniquePath(target.AddExtension(FILE_PATH_LITERAL("part")));
  if (part_path.empty()) {
    return std::nullopt;
  }
  OpenFile open;
  open.file =
      base::File(part_path, base::File::FLAG_CREATE | base::File::FLAG_WRITE);
  if (!open.file.IsValid()) {
    return std::nullopt;
  }
  open.part_path = part_path;
  open.final_path = target;
  return open;
}

bool DirectoryFileSink::Writer::Begin(const std::string& key,
                                      const std::string& name,
                                      const Directories& directories) {
  if (files_.contains(key)) {
    return false;
  }
  std::optional<OpenFile> open = Create(directories.preferred, name);
  if (!open) {
    open = Create(directories.fallback, name);
    if (!open) {
      return false;
    }
    open->used_fallback = !directories.preferred.empty() &&
                          directories.preferred != directories.fallback;
  }
  files_.emplace(key, std::move(*open));
  return true;
}

bool DirectoryFileSink::Writer::Append(const std::string& key,
                                       const std::string& data) {
  auto it = files_.find(key);
  if (it == files_.end()) {
    return false;
  }
  if (!it->second.file.WriteAtCurrentPosAndCheck(base::as_byte_span(data))) {
    Abort(key);
    return false;
  }
  return true;
}

std::optional<SavedFile> DirectoryFileSink::Writer::Commit(
    const std::string& key) {
  auto it = files_.find(key);
  if (it == files_.end()) {
    return std::nullopt;
  }
  OpenFile open = std::move(it->second);
  files_.erase(it);
  const bool flushed = open.file.Flush();
  open.file.Close();
  const base::FilePath final_path = base::GetUniquePath(open.final_path);
  if (!flushed || final_path.empty() ||
      !base::Move(open.part_path, final_path)) {
    base::DeleteFile(open.part_path);
    return std::nullopt;
  }
  if (finalize_) {
    finalize_.Run(final_path);
  }
  return SavedFile{final_path, open.used_fallback};
}

void DirectoryFileSink::Writer::Abort(const std::string& key) {
  auto it = files_.find(key);
  if (it == files_.end()) {
    return;
  }
  it->second.file.Close();
  base::DeleteFile(it->second.part_path);
  files_.erase(it);
}

DirectoryFileSink::DirectoryFileSink(
    DirectoriesCallback directories,
    FinalizeCallback finalize,
    scoped_refptr<base::SequencedTaskRunner> file_task_runner)
    : directories_(std::move(directories)),
      writer_(file_task_runner
                  ? std::move(file_task_runner)
                  : base::ThreadPool::CreateSequencedTaskRunner(
                        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
                         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
              std::move(finalize)) {}

DirectoryFileSink::~DirectoryFileSink() = default;

void DirectoryFileSink::Begin(const std::string& key,
                              const std::string& name,
                              base::OnceClosure on_error) {
  failed_.erase(key);
  error_callbacks_[key] = std::move(on_error);
  writer_.AsyncCall(&Writer::Begin)
      .WithArgs(key, name, directories_.Run())
      .Then(base::BindOnce(&DirectoryFileSink::OnWriteResult,
                           weak_factory_.GetWeakPtr(), key));
}

void DirectoryFileSink::Append(const std::string& key, std::string data) {
  if (failed_.contains(key) || !error_callbacks_.contains(key)) {
    return;
  }
  writer_.AsyncCall(&Writer::Append)
      .WithArgs(key, std::move(data))
      .Then(base::BindOnce(&DirectoryFileSink::OnWriteResult,
                           weak_factory_.GetWeakPtr(), key));
}

void DirectoryFileSink::Commit(const std::string& key,
                               CommitCallback callback) {
  if (failed_.contains(key) || !error_callbacks_.contains(key)) {
    failed_.erase(key);
    error_callbacks_.erase(key);
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::nullopt));
    return;
  }
  writer_.AsyncCall(&Writer::Commit)
      .WithArgs(key)
      .Then(base::BindOnce(&DirectoryFileSink::OnCommitted,
                           weak_factory_.GetWeakPtr(), key,
                           std::move(callback)));
}

void DirectoryFileSink::Abort(const std::string& key) {
  failed_.erase(key);
  error_callbacks_.erase(key);
  writer_.AsyncCall(&Writer::Abort).WithArgs(key);
}

void DirectoryFileSink::OnWriteResult(const std::string& key, bool success) {
  if (!success) {
    Fail(key);
  }
}

void DirectoryFileSink::OnCommitted(const std::string& key,
                                    CommitCallback callback,
                                    std::optional<SavedFile> saved) {
  failed_.erase(key);
  error_callbacks_.erase(key);
  std::move(callback).Run(std::move(saved));
}

void DirectoryFileSink::Fail(const std::string& key) {
  auto it = error_callbacks_.find(key);
  if (it == error_callbacks_.end() || failed_.contains(key)) {
    return;
  }
  failed_.insert(key);
  base::OnceClosure on_error = std::move(it->second);
  if (on_error) {
    std::move(on_error).Run();
  }
}

}  // namespace brave_transfer
