/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sharing_hub/sharing_hub_model.h"

#include "brave/app/brave_command_ids.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

namespace {

bool ShouldAddCopyCleanLinkItem(content::BrowserContext* context) {
  auto* browser =
      chrome::FindBrowserWithProfile(Profile::FromBrowserContext(context));
  if (!browser) {
    return false;
  }
  auto* contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!contents) {
    return false;
  }
  return contents->GetLastCommittedURL().SchemeIsHTTPOrHTTPS();
}

int MaybeAddCopyCleanLinkItem(
    content::BrowserContext* context,
    std::vector<sharing_hub::SharingHubAction>* first_party_action_list) {
  if (ShouldAddCopyCleanLinkItem(context)) {
    first_party_action_list->emplace_back(
        IDC_COPY_CLEAN_LINK,
        l10n_util::GetStringUTF16(IDS_COPY_CLEAN_LINK_SHARING_HUB), &kCopyIcon,
        "SharingHubDesktop.CopyURLSelected", IDS_LINK_COPIED);
  }
  return IDS_SHARING_HUB_COPY_LINK_LABEL;
}

}  // namespace

#undef IDS_SHARING_HUB_COPY_LINK_LABEL
#define IDS_SHARING_HUB_COPY_LINK_LABEL \
  MaybeAddCopyCleanLinkItem(context_, &first_party_action_list_)

#include "src/chrome/browser/sharing_hub/sharing_hub_model.cc"

#undef IDS_SHARING_HUB_COPY_LINK_LABEL
#define IDS_SHARING_HUB_COPY_LINK_LABEL GetOriginalSharingHubCopyLinkID()
