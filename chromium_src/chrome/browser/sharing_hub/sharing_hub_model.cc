/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sharing_hub/sharing_hub_model.h"
#include "brave/app/brave_command_ids.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

namespace {

int GetOriginalSharingHubCopyLinkID() {
  return IDS_SHARING_HUB_COPY_LINK_LABEL;
}

}  // namespace

#undef IDS_SHARING_HUB_COPY_LINK_LABEL
#define IDS_SHARING_HUB_COPY_LINK_LABEL GetOriginalSharingHubCopyLinkID()), \
  &kCopyIcon, true, gfx::ImageSkia(), "SharingHubDesktop.CopyURLSelected"); \
  first_party_action_list_.emplace_back(IDC_COPY_CLEAN_LINK,                \
    l10n_util::GetStringUTF16(IDS_COPY_CLEAN_LINK

#include "src/chrome/browser/sharing_hub/sharing_hub_model.cc"
#undef IDS_SHARING_HUB_COPY_LINK_LABEL
#define IDS_SHARING_HUB_COPY_LINK_LABEL GetOriginalSharingHubCopyLinkID()
