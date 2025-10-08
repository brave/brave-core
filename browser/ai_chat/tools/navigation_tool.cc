// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/navigation_tool.h"

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

constexpr char kPropertyNameWebsiteUrl[] = "website_url";

}  // namespace

NavigationTool::NavigationTool(ContentAgentTaskProvider* task_provider)
    : task_provider_(task_provider) {}

NavigationTool::~NavigationTool() = default;

std::string_view NavigationTool::Name() const {
  return mojom::kNavigateToolName;
}
std::string_view NavigationTool::Description() const {
  return "Navigate the current browser Tab's URL to a new page. Use this "
         "function to completely change the url to another page or website. "
         "The content of the page will be returned as the tool result.";
}

std::optional<base::Value::Dict> NavigationTool::InputProperties() const {
  return CreateInputProperties(
      {{kPropertyNameWebsiteUrl,
        StringProperty(
            "The full website URL to navigate to, starting with https://")}});
}

std::optional<std::vector<std::string>> NavigationTool::RequiredProperties()
    const {
  return std::optional<std::vector<std::string>>({kPropertyNameWebsiteUrl});
}

void NavigationTool::UseTool(const std::string& input_json,
                             UseToolCallback callback) {
  auto input = base::JSONReader::ReadDict(input_json);

  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"));
    return;
  }

  const auto* website_url = input->FindString(kPropertyNameWebsiteUrl);
  if (!website_url) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing 'website_url' property"));
    return;
  }

  GURL url(*website_url);
  if (!url.is_valid()) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: 'website_url' property did not contain a valid URL"));
    return;
  }

  if (!url.SchemeIs(url::kHttpsScheme)) {
    std::move(callback).Run(CreateContentBlocksForText(
        "Error: 'website_url' property must start with https://"));
    return;
  }

  task_provider_->GetOrCreateTabHandleForTask(
      base::BindOnce(&NavigationTool::OnTabHandleCreated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), url));
}

void NavigationTool::OnTabHandleCreated(UseToolCallback callback,
                                        GURL url,
                                        tabs::TabHandle tab_handle) {
  optimization_guide::proto::Actions actions;
  actions.set_task_id(task_provider_->GetTaskId().value());
  auto* action = actions.add_actions();
  action->mutable_navigate()->set_url(url.spec());
  action->mutable_navigate()->set_tab_id(tab_handle.raw_value());
  task_provider_->ExecuteActions(std::move(actions), std::move(callback));
}

}  // namespace ai_chat
