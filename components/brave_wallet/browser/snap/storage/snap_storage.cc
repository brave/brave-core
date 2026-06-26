/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_storage.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"

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

}  // namespace

SnapStorage::SnapStorage(
    const base::FilePath& snaps_dir,
    scoped_refptr<base::SequencedTaskRunner> origin_task_runner)
    : snaps_dir_(snaps_dir),
      origin_task_runner_(std::move(origin_task_runner)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

SnapStorage::~SnapStorage() = default;

bool SnapStorage::MoveSnapFiles(const std::string& snap_id,
                                const base::FilePath& unpacked_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!origin_task_runner_->RunsTasksInCurrentSequence());

  base::FilePath dest_dir = GetSnapDir(snap_id);
  if (!base::CreateDirectory(dest_dir)) {
    return false;
  }
  if (!base::Move(unpacked_dir.AppendASCII("bundle.js"),
                  dest_dir.Append(kBundleFileName))) {
    return false;
  }
  if (!base::Move(unpacked_dir.AppendASCII("manifest.json"),
                  dest_dir.Append(kManifestFileName))) {
    return false;
  }
  return true;
}

std::optional<std::string> SnapStorage::ReadBundle(
    const std::string& snap_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!origin_task_runner_->RunsTasksInCurrentSequence());

  base::FilePath snap_dir = GetSnapDir(snap_id);
  std::string contents;
  if (!base::ReadFileToString(snap_dir.Append(kBundleFileName), &contents)) {
    return std::nullopt;
  }
  return contents;
}

bool SnapStorage::DeleteSnap(const std::string& snap_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!origin_task_runner_->RunsTasksInCurrentSequence());

  base::FilePath snap_dir = GetSnapDir(snap_id);
  return base::DeletePathRecursively(snap_dir);
}

bool SnapStorage::WriteState(const std::string& snap_id,
                             const std::string& json) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!origin_task_runner_->RunsTasksInCurrentSequence());

  base::FilePath snap_dir = GetSnapDir(snap_id);
  if (!base::CreateDirectory(snap_dir)) {
    return false;
  }
  return base::WriteFile(snap_dir.Append(kStateFileName), json);
}

std::optional<std::string> SnapStorage::ReadState(
    const std::string& snap_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!origin_task_runner_->RunsTasksInCurrentSequence());

  base::FilePath snap_dir = GetSnapDir(snap_id);
  std::string contents;
  if (!base::ReadFileToString(snap_dir.Append(kStateFileName), &contents)) {
    return std::nullopt;
  }
  return contents;
}

bool SnapStorage::HasSnap(const std::string& snap_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!origin_task_runner_->RunsTasksInCurrentSequence());

  return base::PathExists(GetSnapDir(snap_id).Append(kBundleFileName));
}

base::FilePath SnapStorage::GetSnapDir(const std::string& snap_id) const {
  return snaps_dir_.AppendASCII(SnapIdToDir(snap_id));
}

}  // namespace brave_wallet
