// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/select_tool.h"

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

SelectTool::SelectTool(BrowserToolTaskProvider* task_provider,
                       actor::ActorKeyedService* actor_service)
    : actor_service_(actor_service), task_provider_(task_provider) {}

SelectTool::~SelectTool() = default;

std::string_view SelectTool::Name() const {
  return "select_dropdown";
}

std::string_view SelectTool::Description() const {
  return "Select an option from a dropdown menu (<select> element) in the "
         "current web page. You can specify either a content node ID with "
         "document identifier, or x/y coordinates to identify the dropdown. "
         "The value should match the 'value' attribute of the desired option.";
}

std::optional<base::Value::Dict> SelectTool::InputProperties() const {
  return CreateInputProperties(
      {{"content_node_id",
        IntegerProperty("The DOM node ID of the <select> element. Use this "
                        "OR coordinates, not both.")},
       {"document_identifier",
        StringProperty("Document identifier for the content node. Required "
                       "when using content_node_id.")},
       {"x",
        NumberProperty(
            "X coordinate of the dropdown (if not using content_node_id)")},
       {"y",
        NumberProperty(
            "Y coordinate of the dropdown (if not using content_node_id)")},
       {"value",
        StringProperty("The value attribute of the <option> to select")}});
}

std::optional<std::vector<std::string>> SelectTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>({"value"});
}

void SelectTool::UseTool(const std::string& input_json,
                         UseToolCallback callback,
                         std::optional<base::Value> client_data) {
  data_decoder::DataDecoder::ParseJsonIsolated(
      input_json,
      base::BindOnce(&SelectTool::OnParseJson, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void SelectTool::OnParseJson(UseToolCallback callback,
                             data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value() || !result->is_dict()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Failed to parse input JSON. Please try again."));
    return;
  }

  const auto& dict = result->GetDict();

  // Validate value parameter
  const auto* value = dict.FindString("value");
  if (!value) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing required parameter 'value' - the "
                                   "value attribute of the option to select."));
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
        &SelectTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
        std::move(callback), content_node_id.value(), *document_identifier, 0.0,
        0.0, *value));
  } else if (x.has_value() && y.has_value()) {
    // Using coordinate approach
    task_provider_->GetOrCreateTabHandleForTask(base::BindOnce(
        &SelectTool::OnTabHandleCreated, weak_ptr_factory_.GetWeakPtr(),
        std::move(callback), -1, "", x.value(), y.value(), *value));
  } else {
    std::move(callback).Run(CreateContentBlocksForText(
        "Either content_node_id with document_identifier OR x,y coordinates "
        "must be provided."));
    return;
  }
}

void SelectTool::OnTabHandleCreated(UseToolCallback callback,
                                    int content_node_id,
                                    std::string document_identifier,
                                    double x,
                                    double y,
                                    std::string value,
                                    tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* select_action = action->mutable_select();
  select_action->set_tab_id(tab_handle.raw_value());
  select_action->set_value(value);

  // Set target
  auto* target = select_action->mutable_target();
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

  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
