// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_disk_cache.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/remote_models_serialization.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {

namespace {

void WriteToFile(const base::FilePath& path, std::string content) {
  base::AssertBlockingAllowed();
  if (!base::WriteFile(path, content)) {
    DVLOG(1) << "RemoteModelsDiskCache: failed to write " << path;
  }
}

std::optional<std::vector<mojom::ModelPtr>> ReadAndParseFromFile(
    const base::FilePath& path) {
  base::AssertBlockingAllowed();
  std::string content;
  if (!base::ReadFileToString(path, &content)) {
    return std::nullopt;
  }

  auto parsed = base::JSONReader::ReadDict(content, base::JSON_PARSE_RFC);
  if (!parsed) {
    DVLOG(1) << "RemoteModelsDiskCache: failed to parse cache JSON";
    return std::nullopt;
  }

  std::vector<mojom::ModelPtr> models =
      ParseModelsFromJSON(base::Value(std::move(*parsed)));
  if (models.empty()) {
    DVLOG(1) << "RemoteModelsDiskCache: cache contained no valid models";
    return std::nullopt;
  }

  return models;
}

std::string SerializeCache(const std::vector<mojom::ModelPtr>& models) {
  base::DictValue root;
  root.Set(kModelsKey, SerializeModels(models));
  return base::WriteJson(root).value_or("");
}

}  // namespace

RemoteModelsDiskCache::RemoteModelsDiskCache(base::FilePath path,
                                             base::TimeDelta ttl,
                                             PrefService* pref_service)
    : path_(std::move(path)), ttl_(ttl), pref_service_(pref_service) {}

RemoteModelsDiskCache::~RemoteModelsDiskCache() = default;

void RemoteModelsDiskCache::Load(LoadCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const base::Time cached_at =
      pref_service_->GetTime(prefs::kRemoteModelsCachedAt);
  if (cached_at.is_null() || base::Time::Now() - cached_at > ttl_) {
    DVLOG(1) << "RemoteModelsDiskCache: cache absent or expired";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::nullopt));
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadAndParseFromFile, path_), std::move(callback));
}

void RemoteModelsDiskCache::Save(std::vector<mojom::ModelPtr> models,
                                 base::OnceClosure on_complete) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  pref_service_->SetTime(prefs::kRemoteModelsCachedAt, base::Time::Now());
  base::ThreadPool::PostTaskAndReply(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&WriteToFile, path_, SerializeCache(models)),
      std::move(on_complete));
}

}  // namespace ai_chat
