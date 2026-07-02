// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_cache.h"

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
#include "base/numerics/safe_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/remote_models_fetcher.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

namespace {

constexpr char kFetchedAtKey[] = "fetched_at";
constexpr char kModelsKey[] = "models";

// Field names must match the server JSON format so that
// RemoteModelsFetcher::ParseModelsFromJSON can re-parse the cached data.
constexpr char kKeyField[] = "key";
constexpr char kDisplayNameField[] = "display_name";
constexpr char kCapabilitiesField[] = "capabilities";
constexpr char kIsSuggestedModelField[] = "is_suggested_model";
constexpr char kIsNearModelField[] = "is_near_model";
constexpr char kOptionsField[] = "options";
constexpr char kTypeField[] = "type";
constexpr char kLeoType[] = "leo";
constexpr char kNameField[] = "name";
constexpr char kDisplayMakerField[] = "display_maker";
constexpr char kDescriptionField[] = "description";
constexpr char kAccessField[] = "access";
constexpr char kMaxAssociatedContentLengthField[] =
    "max_associated_content_length";
constexpr char kLongConversationWarningCharacterLimitField[] =
    "long_conversation_warning_character_limit";

std::string AccessToString(mojom::ModelAccess access) {
  switch (access) {
    case mojom::ModelAccess::BASIC:
      return "basic";
    case mojom::ModelAccess::PREMIUM:
      return "premium";
    case mojom::ModelAccess::BASIC_AND_PREMIUM:
      return "basic_and_premium";
  }
}

std::string CapabilityToString(mojom::ConversationCapability capability) {
  switch (capability) {
    case mojom::ConversationCapability::CHAT:
      return "chat";
    case mojom::ConversationCapability::CONTENT_AGENT:
      return "content_agent";
    case mojom::ConversationCapability::DEEP_RESEARCH:
      return "deep_research";
    case mojom::ConversationCapability::FILES:
      return "files";
    case mojom::ConversationCapability::SUMMARY:
      return "summary";
  }
}

base::DictValue ModelToDict(const mojom::Model& model) {
  base::DictValue dict;
  dict.Set(kKeyField, model.key);
  dict.Set(kDisplayNameField, model.display_name);
  dict.Set(kIsSuggestedModelField, model.is_suggested_model);
  dict.Set(kIsNearModelField, model.is_near_model);

  base::ListValue capabilities;
  for (auto capability : model.supported_capabilities) {
    capabilities.Append(CapabilityToString(capability));
  }
  dict.Set(kCapabilitiesField, std::move(capabilities));

  if (model.options && model.options->is_leo_model_options()) {
    const auto& leo_opts = model.options->get_leo_model_options();
    base::DictValue options;
    options.Set(kTypeField, kLeoType);
    options.Set(kNameField, leo_opts->name);
    if (!leo_opts->display_maker.empty()) {
      options.Set(kDisplayMakerField, leo_opts->display_maker);
    }
    options.Set(kDescriptionField, leo_opts->description);
    options.Set(kAccessField, AccessToString(leo_opts->access));
    options.Set(
        kMaxAssociatedContentLengthField,
        base::saturated_cast<int>(leo_opts->max_associated_content_length));
    options.Set(kLongConversationWarningCharacterLimitField,
                base::saturated_cast<int>(
                    leo_opts->long_conversation_warning_character_limit));
    dict.Set(kOptionsField, std::move(options));
  }

  return dict;
}

std::string SerializeCache(const std::vector<mojom::ModelPtr>& models) {
  base::ListValue models_list;
  for (const auto& model : models) {
    if (model) {
      models_list.Append(ModelToDict(*model));
    }
  }

  base::DictValue root;
  root.Set(kFetchedAtKey, base::Time::Now().InSecondsFSinceUnixEpoch());
  root.Set(kModelsKey, std::move(models_list));

  return base::WriteJson(root).value_or("");
}

void WriteToFile(const base::FilePath& path, std::string content) {
  base::AssertBlockingAllowed();
  if (!base::WriteFile(path, content)) {
    DVLOG(1) << "RemoteModelsCache: failed to write " << path;
  }
}

std::optional<std::vector<mojom::ModelPtr>> ReadAndParseFromFile(
    const base::FilePath& path,
    base::TimeDelta ttl) {
  base::AssertBlockingAllowed();
  std::string content;
  if (!base::ReadFileToString(path, &content)) {
    return std::nullopt;
  }

  auto parsed = base::JSONReader::ReadDict(content, base::JSON_PARSE_RFC);
  if (!parsed) {
    DVLOG(1) << "RemoteModelsCache: failed to parse cache JSON";
    return std::nullopt;
  }

  std::optional<double> fetched_at = parsed->FindDouble(kFetchedAtKey);
  if (!fetched_at) {
    return std::nullopt;
  }

  if (base::Time::Now() - base::Time::FromSecondsSinceUnixEpoch(*fetched_at) >
      ttl) {
    DVLOG(1) << "RemoteModelsCache: cache expired";
    return std::nullopt;
  }

  std::vector<mojom::ModelPtr> models =
      RemoteModelsFetcher::ParseModelsFromJSON(base::Value(std::move(*parsed)));
  if (models.empty()) {
    DVLOG(1) << "RemoteModelsCache: cache contained no valid models";
    return std::nullopt;
  }

  return models;
}

}  // namespace

RemoteModelsCache::RemoteModelsCache(base::FilePath path, base::TimeDelta ttl)
    : path_(std::move(path)), ttl_(ttl) {}

RemoteModelsCache::~RemoteModelsCache() = default;

void RemoteModelsCache::Load(LoadCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadAndParseFromFile, path_, ttl_), std::move(callback));
}

void RemoteModelsCache::Save(std::vector<mojom::ModelPtr> models,
                             base::OnceClosure on_complete) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::ThreadPool::PostTaskAndReply(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&WriteToFile, path_, SerializeCache(models)),
      std::move(on_complete));
}

}  // namespace ai_chat
