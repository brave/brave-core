/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sharing_hub/brave_sharing_hub_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/brave_talk/brave_talk_service.h"
#include "brave/browser/brave_talk/brave_talk_service_factory.h"
#include "brave/components/l10n/common/locale_util.h"
#include "chrome/browser/sharing_hub/sharing_hub_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"

namespace sharing_hub {

BraveSharingHubModel::BraveSharingHubModel(content::BrowserContext* context)
    : SharingHubModel(context),
      brave_talk_share_tab_action_({IDC_BRAVE_TALK_SHARE_TAB,
                                    brave_l10n::GetLocalizedResourceUTF16String(
                                        IDS_BRAVE_TALK_SHARE_TAB_BUTTON_TOOLTIP),
                                    &vector_icons::kScreenShareIcon, true,
                                    gfx::ImageSkia(), "BraveTalk.ShareTab"}) {}

BraveSharingHubModel::~BraveSharingHubModel() = default;

void BraveSharingHubModel::GetFirstPartyActionList(
    content::WebContents* web_contents,
    std::vector<SharingHubAction>* list) {
  auto* brave_talk_service = brave_talk::BraveTalkServiceFactory::GetForContext(
      web_contents->GetBrowserContext());
  if (brave_talk_service->is_requesting_tab()) {
    list->push_back(brave_talk_share_tab_action_);
  }

  SharingHubModel::GetFirstPartyActionList(web_contents, list);
}

}  // namespace sharing_hub
