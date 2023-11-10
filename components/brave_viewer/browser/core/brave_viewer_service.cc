// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_viewer/browser/core/brave_viewer_service.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_viewer/common/features.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_viewer {

namespace {

const base::FilePath::CharType kTestScript[] = FILE_PATH_LITERAL("test.js");

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot "
            << "read file " << file_path;
  }
  return contents;
}

}  // namespace

// static
BraveViewerService* BraveViewerService::GetInstance() {
  // Check if feature flag is enabled.
  if (!base::FeatureList::IsEnabled(brave_viewer::features::kBraveViewer)) {
    return nullptr;
  }
  return base::Singleton<BraveViewerService>::get();
}

BraveViewerService::BraveViewerService() = default;

BraveViewerService::~BraveViewerService() = default;

void BraveViewerService::GetTestScript(
    const GURL& url,
    base::OnceCallback<void(std::string test_script)> cb) const {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(ReadFile,
                     base::FilePath(component_path_).Append(kTestScript)),
      std::move(cb));
  return;
}

void BraveViewerService::LoadNewComponentVersion(const base::FilePath& path) {
  SetComponentPath(path);
  // TODO - keep rules loaded in memory, or load async as needed?
}

void BraveViewerService::SetComponentPath(const base::FilePath& path) {
  component_path_ = path;
}

}  // namespace brave_viewer
