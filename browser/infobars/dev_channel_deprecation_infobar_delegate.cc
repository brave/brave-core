/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/dev_channel_deprecation_infobar_delegate.h"

#include <memory>

#include "brave/browser/infobars/brave_confirm_infobar_creator.h"
#include "brave/components/constants/url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/common/channel_info.h"
#include "components/infobars/core/infobar.h"
#include "components/strings/grit/components_strings.h"
#include "components/version_info/channel.h"
#include "ui/base/l10n/l10n_util.h"

// static
void DevChannelDeprecationInfoBarDelegate::CreateIfNeeded(
    infobars::InfoBarManager* infobar_manager) {
  if (chrome::GetChannel() == version_info::Channel::DEV) {
    infobar_manager->AddInfoBar(
        CreateBraveConfirmInfoBar(std::unique_ptr<BraveConfirmInfoBarDelegate>(
            new DevChannelDeprecationInfoBarDelegate())));
  }
}

DevChannelDeprecationInfoBarDelegate::~DevChannelDeprecationInfoBarDelegate() =
    default;

DevChannelDeprecationInfoBarDelegate::DevChannelDeprecationInfoBarDelegate() =
    default;

infobars::InfoBarDelegate::InfoBarIdentifier
DevChannelDeprecationInfoBarDelegate::GetIdentifier() const {
  return DEV_CHANNEL_DEPRECATION_INFOBAR_DELEGATE;
}

std::u16string DevChannelDeprecationInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_DEV_CHANNEL_DEPRECATION_INFOBAR_MESSAGE);
}

int DevChannelDeprecationInfoBarDelegate::GetButtons() const {
  return BUTTON_NONE;
}

std::vector<int> DevChannelDeprecationInfoBarDelegate::GetButtonsOrder() const {
  return {};
}

bool DevChannelDeprecationInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  // Since deprecation infobar communicates critical state, it should persist
  // until explicitly dismissed.
  return false;
}

std::u16string DevChannelDeprecationInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

GURL DevChannelDeprecationInfoBarDelegate::GetLinkURL() const {
  return GURL(kDevChannelDeprecationLearnMoreUrl);
}
