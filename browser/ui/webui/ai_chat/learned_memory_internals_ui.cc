// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/learned_memory_internals_ui.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/user_memory_manager.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/resources/grit/ai_chat_ui_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace ai_chat {

namespace {

UserMemoryManager* GetUserMemoryManager(Profile* profile) {
  AIChatService* service = AIChatServiceFactory::GetForBrowserContext(profile);
  return service ? service->GetUserMemoryManager() : nullptr;
}

std::string TriggerName(DreamingTrigger trigger) {
  switch (trigger) {
    case DreamingTrigger::kTimer:
      return "timer";
    case DreamingTrigger::kDreamNow:
      return "dream_now";
    case DreamingTrigger::kEval:
      return "eval";
  }
}

}  // namespace

LearnedMemoryInternalsUIConfig::LearnedMemoryInternalsUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kLeoMemoryInternalsHost) {}

bool LearnedMemoryInternalsUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return base::FeatureList::IsEnabled(features::kAIChatLearnedMemory) &&
         AIChatServiceFactory::GetForBrowserContext(browser_context);
}

LearnedMemoryInternalsUI::LearnedMemoryInternalsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui), profile_(Profile::FromWebUI(web_ui)) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile_, kLeoMemoryInternalsHost);
  webui::SetupWebUIDataSource(source, kAiChatUiGenerated,
                              IDR_AI_CHAT_LEARNED_MEMORY_INTERNALS_HTML);
}

LearnedMemoryInternalsUI::~LearnedMemoryInternalsUI() = default;

void LearnedMemoryInternalsUI::BindInterface(
    mojo::PendingReceiver<mojom::LearnedMemoryInternals> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void LearnedMemoryInternalsUI::GetDreamingRuns(
    GetDreamingRunsCallback callback) {
  UserMemoryManager* manager = GetUserMemoryManager(profile_);
  if (!manager) {
    std::move(callback).Run(false, {});
    return;
  }
  std::vector<mojom::DreamingRunRecordPtr> runs;
  for (DreamingRunRecord& record : manager->GetDreamingRunRecords()) {
    runs.push_back(mojom::DreamingRunRecord::New(
        record.id, TriggerName(record.trigger), record.start_time,
        record.end_time,
        record.status ? DreamingStatusToString(*record.status) : "running",
        base::WriteJson(record.trace).value_or("[]")));
  }
  std::move(callback).Run(true, std::move(runs));
}

void LearnedMemoryInternalsUI::GetTurnLookups(GetTurnLookupsCallback callback) {
  UserMemoryManager* manager = GetUserMemoryManager(profile_);
  if (!manager) {
    std::move(callback).Run(false, {});
    return;
  }
  std::vector<mojom::TurnLookupRecordPtr> lookups;
  for (TurnLookupRecord& record : manager->GetTurnLookupRecords()) {
    lookups.push_back(mojom::TurnLookupRecord::New(
        record.id, record.conversation_uuid, record.start_time, record.end_time,
        base::WriteJson(record.trace).value_or("[]"),
        base::WriteJson(record.decision_requests).value_or("[]")));
  }
  std::move(callback).Run(true, std::move(lookups));
}

WEB_UI_CONTROLLER_TYPE_IMPL(LearnedMemoryInternalsUI)

}  // namespace ai_chat
