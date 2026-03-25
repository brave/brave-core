/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/snap_storage.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"

namespace brave_wallet {

namespace {

constexpr base::FilePath::CharType kBundleFileName[] =
    FILE_PATH_LITERAL("bundle.js");
constexpr base::FilePath::CharType kManifestFileName[] =
    FILE_PATH_LITERAL("manifest.json");
constexpr base::FilePath::CharType kStateFileName[] =
    FILE_PATH_LITERAL("state.json");

// Converts a snap_id (e.g. "npm:@polkagate/snap") to a filesystem-safe
// directory name (e.g. "npm_@polkagate_snap") by replacing ':' and '/' with
// '_'.
std::string SnapIdToDir(const std::string& snap_id) {
  std::string result;
  base::ReplaceChars(snap_id, ":/", "_", &result);
  return result;
}

// Writes bundle.js and manifest.json to |snap_dir|. Returns true on success.
// Runs on a thread-pool worker.
bool SaveSnapFiles(const base::FilePath& snap_dir,
                   const std::string& bundle_js,
                   const std::string& manifest_json) {
  if (!base::CreateDirectory(snap_dir)) {
    return false;
  }
  if (!base::WriteFile(snap_dir.Append(kBundleFileName), bundle_js)) {
    return false;
  }
  if (!base::WriteFile(snap_dir.Append(kManifestFileName), manifest_json)) {
    return false;
  }
  return true;
}

// Reads bundle.js from |snap_dir|. Returns std::nullopt on error.
// Runs on a thread-pool worker.
std::optional<std::string> ReadBundleFile(const base::FilePath& snap_dir) {
  std::string contents;
  if (!base::ReadFileToString(snap_dir.Append(kBundleFileName), &contents)) {
    return std::nullopt;
  }
  return contents;
}

// Deletes |path| recursively. Runs on a thread-pool worker.
void DeleteSnapDir(const base::FilePath& path) {
  base::DeletePathRecursively(path);
}

// Writes |json| to state.json under |snap_dir|, creating the directory first.
// Returns true on success. Runs on a thread-pool worker.
bool WriteStateFile(const base::FilePath& snap_dir, const std::string& json) {
  if (!base::CreateDirectory(snap_dir)) {
    return false;
  }
  return base::WriteFile(snap_dir.Append(kStateFileName), json);
}

// Reads state.json from |snap_dir|. Returns std::nullopt on error or missing
// file. Runs on a thread-pool worker.
std::optional<std::string> ReadStateFile(const base::FilePath& snap_dir) {
  std::string contents;
  if (!base::ReadFileToString(snap_dir.Append(kStateFileName), &contents)) {
    return std::nullopt;
  }
  return contents;
}

}  // namespace

SnapStorage::SnapStorage(const base::FilePath& profile_path)
    : snaps_dir_(profile_path.Append(FILE_PATH_LITERAL("BraveWallet"))
                     .Append(FILE_PATH_LITERAL("Snaps"))) {}

SnapStorage::~SnapStorage() = default;

void SnapStorage::SaveSnap(const std::string& snap_id,
                           const std::string& bundle_js,
                           const std::string& manifest_json,
                           base::OnceCallback<void(bool)> on_done) {
  base::FilePath snap_dir = GetSnapDir(snap_id);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&SaveSnapFiles, snap_dir, bundle_js, manifest_json),
      std::move(on_done));
}

void SnapStorage::ReadBundle(
    const std::string& snap_id,
    base::OnceCallback<void(std::optional<std::string>)> cb) {
  base::FilePath snap_dir = GetSnapDir(snap_id);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadBundleFile, snap_dir),
      std::move(cb));
}

void SnapStorage::DeleteSnap(const std::string& snap_id) {
  base::FilePath snap_dir = GetSnapDir(snap_id);
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&DeleteSnapDir, snap_dir));
}

void SnapStorage::WriteState(const std::string& snap_id,
                             const std::string& json,
                             base::OnceCallback<void(bool)> on_done) {
  base::FilePath snap_dir = GetSnapDir(snap_id);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&WriteStateFile, snap_dir, json),
      std::move(on_done));
}

void SnapStorage::ReadState(
    const std::string& snap_id,
    base::OnceCallback<void(std::optional<std::string>)> cb) {
  base::FilePath snap_dir = GetSnapDir(snap_id);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadStateFile, snap_dir),
      std::move(cb));
}

bool SnapStorage::HasSnap(const std::string& snap_id) const {
  return base::PathExists(GetSnapDir(snap_id).Append(kBundleFileName));
}

base::FilePath SnapStorage::GetSnapDir(const std::string& snap_id) const {
  return snaps_dir_.AppendASCII(SnapIdToDir(snap_id));
}

}  // namespace brave_wallet
