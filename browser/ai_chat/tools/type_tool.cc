// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/type_tool.h"

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

TypeTool::TypeTool(BrowserToolTaskProvider* task_provider,
                   actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), task_provider_(task_provider) {}

TypeTool::~TypeTool() = default;

std::string_view TypeTool::Name() const {
  return "type_text";
}

std::string_view TypeTool::Description() const {
  return "Type text into an input field on the current web page. Element must "
         "be editable and focusable. Prefer input over container elements."
         "You can specify either a content node ID with document identifier, "
         "or x/y "
         "coordinates. Supports different modes for handling existing text and "
         "can "
         "optionally press Enter after typing.";
}

std::optional<base::Value::Dict> TypeTool::InputProperties() const {
  return CreateInputProperties(
      {{"content_node_id",
        IntegerProperty("The DOM node ID of the element to type into. Use this "
                        "OR coordinates, not both.")},
       {"document_identifier",
        StringProperty("Document identifier for the content node. Required "
                       "when using content_node_id.")},
       {"x", NumberProperty(
                 "X coordinate to type at (if not using content_node_id)")},
       {"y", NumberProperty(
                 "Y coordinate to type at (if not using content_node_id)")},
       {"text", StringProperty("The text to type into the element")},
       {"follow_by_enter",
        BooleanProperty("Whether to press Enter after typing the text")},
       {"mode", StringProperty("How to handle existing text in the element",
                               std::vector<std::string>{"replace", "prepend",
                                                        "append"})}});
}

std::optional<std::vector<std::string>> TypeTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>(
      {"text", "follow_by_enter", "mode"});
}

void TypeTool::UseTool(const std::string& input_json,
                       UseToolCallback callback,
                       std::optional<base::Value> client_data) {
  data_decoder::DataDecoder::ParseJsonIsolated(
      input_json,
      base::BindOnce(&TypeTool::OnParseJson, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void TypeTool::OnParseJson(UseToolCallback callback,
                           data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value() || !result->is_dict()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Failed to parse input JSON. Please try again."));
    return;
  }

  const auto& dict = result->GetDict();

  // Validate required fields
  const auto* text = dict.FindString("text");
  std::optional<bool> follow_by_enter = dict.FindBool("follow_by_enter");
  const auto* mode = dict.FindString("mode");

  if (!text) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing required field: text"));
    return;
  }

  if (!follow_by_enter.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing required field: follow_by_enter"));
    return;
  }

  if (!mode ||
      (*mode != "replace" && *mode != "prepend" && *mode != "append")) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Invalid or missing mode. Must be 'replace', 'prepend', or 'append'."));
    return;
  }

  // Check if using content_node_id approach
  std::optional<int> content_node_id = dict.FindInt("content_node_id");
  const auto* document_identifier = dict.FindString("document_identifier");

  // Check if using coordinate approach
  std::optional<double> x = dict.FindDouble("x");
  std::optional<double> y = dict.FindDouble("y");

  if (content_node_id.has_value()) {
    // Using content node approach
    if (!document_identifier) {
      std::move(callback).Run(CreateContentBlocksForText(
          "document_identifier is required when using content_node_id."));
      return;
    }

    task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
        &TypeTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
        std::move(callback), content_node_id.value(), *document_identifier, 0.0,
        0.0, *text, follow_by_enter.value(), *mode));
  } else if (x.has_value() && y.has_value()) {
    // Using coordinate approach
    task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
        &TypeTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
        std::move(callback), -1, "", x.value(), y.value(), *text,
        follow_by_enter.value(), *mode));
  } else {
    std::move(callback).Run(CreateContentBlocksForText(
        "Either content_node_id with document_identifier OR x,y coordinates "
        "must be provided."));
    return;
  }
}

void TypeTool::OnTabHandleCreated(UseToolCallback callback,
                                  int content_node_id,
                                  std::string document_identifier,
                                  double x,
                                  double y,
                                  std::string text,
                                  bool follow_by_enter,
                                  std::string mode,
                                  tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* type_action = action->mutable_type();
  type_action->set_tab_id(tab_handle.raw_value());

  // Set target
  auto* target = type_action->mutable_target();
  if (content_node_id != -1) {
    // Using content node
    target->set_content_node_id(content_node_id);
    auto* doc_id = target->mutable_document_identifier();
    doc_id->set_serialized_token(document_identifier);
  } else {
    // Using coordinates
    auto* coordinate = target->mutable_coordinate();
    coordinate->set_x(static_cast<int32_t>(x));
    coordinate->set_y(static_cast<int32_t>(y));
  }

  // Set text content
  type_action->set_text(text);
  type_action->set_follow_by_enter(follow_by_enter);

  // Set type mode
  if (mode == "replace") {
    type_action->set_mode(
        optimization_guide::proto::TypeAction::DELETE_EXISTING);
  } else if (mode == "prepend") {
    type_action->set_mode(optimization_guide::proto::TypeAction::PREPEND);
  } else if (mode == "append") {
    type_action->set_mode(optimization_guide::proto::TypeAction::APPEND);
  }

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
