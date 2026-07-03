// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/remote_models_serialization.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"

namespace ai_chat {

namespace {

std::optional<mojom::ModelAccess> ParseAccess(const std::string& access_str) {
  if (access_str == "basic") {
    return mojom::ModelAccess::BASIC;
  }
  if (access_str == "premium") {
    return mojom::ModelAccess::PREMIUM;
  }
  if (access_str == "basic_and_premium") {
    return mojom::ModelAccess::BASIC_AND_PREMIUM;
  }
  DVLOG(1) << "Unknown model access: " << access_str;
  return std::nullopt;
}

std::optional<mojom::ConversationCapability> ParseCapability(
    const std::string& capability_str) {
  if (capability_str == "chat") {
    return mojom::ConversationCapability::CHAT;
  }
  if (capability_str == "content_agent") {
    return mojom::ConversationCapability::CONTENT_AGENT;
  }
  if (capability_str == "deep_research") {
    return mojom::ConversationCapability::DEEP_RESEARCH;
  }
  if (capability_str == "files") {
    return mojom::ConversationCapability::FILES;
  }
  if (capability_str == "summary") {
    return mojom::ConversationCapability::SUMMARY;
  }
  DVLOG(1) << "Unknown conversation capability: " << capability_str;
  return std::nullopt;
}

struct ParsedCapabilities {
  std::vector<mojom::ConversationCapability> capabilities;
  mojom::ModelCategory category;
};

// Parses the model's capability list and derives its category from the first
// CHAT or SUMMARY capability in list order. Returns std::nullopt if the list
// is missing or declares neither category capability, in which case the model
// is rejected.
std::optional<ParsedCapabilities> ParseCapabilities(
    const base::DictValue& model_dict) {
  const base::ListValue* capabilities_list =
      model_dict.FindList(kCapabilitiesField);
  if (!capabilities_list) {
    return std::nullopt;
  }

  std::vector<mojom::ConversationCapability> capabilities;
  for (const auto& capability_value : *capabilities_list) {
    if (!capability_value.is_string()) {
      continue;
    }
    if (auto capability = ParseCapability(capability_value.GetString())) {
      capabilities.push_back(*capability);
    }
  }

  for (auto capability : capabilities) {
    if (capability == mojom::ConversationCapability::CHAT) {
      return ParsedCapabilities{std::move(capabilities),
                                mojom::ModelCategory::CHAT};
    }
    if (capability == mojom::ConversationCapability::SUMMARY) {
      return ParsedCapabilities{std::move(capabilities),
                                mojom::ModelCategory::SUMMARY};
    }
  }
  return std::nullopt;
}

mojom::ModelPtr ParseModel(const base::DictValue& model_dict) {
  const std::string* key = model_dict.FindString(kKeyField);
  if (!key || key->empty()) {
    return nullptr;
  }

  const std::string* display_name = model_dict.FindString(kDisplayNameField);
  if (!display_name || display_name->empty()) {
    return nullptr;
  }

  const base::DictValue* options_dict = model_dict.FindDict(kOptionsField);
  if (!options_dict) {
    return nullptr;
  }

  const std::string* type = options_dict->FindString(kTypeField);
  if (!type || *type != kLeoType) {
    return nullptr;
  }

  const std::string* name = options_dict->FindString(kNameField);
  if (!name || name->empty()) {
    return nullptr;
  }

  std::optional<int> max_content_length =
      options_dict->FindInt(kMaxAssociatedContentLengthField);
  if (max_content_length.has_value() && *max_content_length <= 0) {
    return nullptr;
  }

  std::optional<int> warning_limit =
      options_dict->FindInt(kLongConversationWarningCharacterLimitField);
  if (warning_limit.has_value() && *warning_limit <= 0) {
    return nullptr;
  }

  const std::string* access = options_dict->FindString(kAccessField);
  std::optional<mojom::ModelAccess> parsed_access =
      access ? ParseAccess(*access) : std::nullopt;
  if (!parsed_access.has_value()) {
    return nullptr;
  }

  auto parsed_capabilities = ParseCapabilities(model_dict);
  if (!parsed_capabilities) {
    return nullptr;
  }

  auto model = mojom::Model::New();
  model->key = *key;
  model->display_name = *display_name;
  model->is_suggested_model =
      model_dict.FindBool(kIsSuggestedModelField).value_or(false);
  model->is_near_model = model_dict.FindBool(kIsNearModelField).value_or(false);
  model->supported_capabilities = std::move(parsed_capabilities->capabilities);

  auto leo_opts = mojom::LeoModelOptions::New();
  leo_opts->name = *name;

  const std::string* display_maker =
      options_dict->FindString(kDisplayMakerField);
  if (display_maker) {
    leo_opts->display_maker = *display_maker;
  }

  const std::string* description = options_dict->FindString(kDescriptionField);
  leo_opts->description = description ? *description : "";

  leo_opts->category = parsed_capabilities->category;
  leo_opts->access = *parsed_access;

  if (!max_content_length.has_value()) {
    max_content_length =
        (leo_opts->access == mojom::ModelAccess::PREMIUM) ? 90000 : 32000;
  }
  if (!warning_limit.has_value()) {
    warning_limit =
        (leo_opts->access == mojom::ModelAccess::PREMIUM) ? 160000 : 51200;
  }

  leo_opts->max_associated_content_length =
      base::saturated_cast<uint32_t>(*max_content_length);
  leo_opts->long_conversation_warning_character_limit =
      base::saturated_cast<uint32_t>(*warning_limit);

  model->options = mojom::ModelOptions::NewLeoModelOptions(std::move(leo_opts));

  return model;
}

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

}  // namespace

std::vector<mojom::ModelPtr> ParseModelsFromJSON(const base::Value& json) {
  const base::ListValue* models_list = nullptr;

  if (json.is_dict()) {
    models_list = json.GetDict().FindList(kModelsKey);
  } else if (json.is_list()) {
    models_list = &json.GetList();
  }

  if (!models_list) {
    return {};
  }

  std::vector<mojom::ModelPtr> models;
  for (const auto& model_value : *models_list) {
    if (!model_value.is_dict()) {
      continue;
    }
    auto model = ParseModel(model_value.GetDict());
    if (model) {
      models.push_back(std::move(model));
    }
  }
  return models;
}

base::ListValue SerializeModels(const std::vector<mojom::ModelPtr>& models) {
  base::ListValue list;
  for (const auto& model : models) {
    if (model) {
      list.Append(ModelToDict(*model));
    }
  }
  return list;
}

}  // namespace ai_chat
