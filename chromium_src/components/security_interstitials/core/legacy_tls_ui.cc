/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/core/legacy_tls_ui.h"

#define HandleCommand HandleCommand_ChromiumImpl
#include "../../../../../components/security_interstitials/core/legacy_tls_ui.cc"
#undef HandleCommand

namespace security_interstitials {
namespace {
constexpr char kHelpArticle[] = "hc/en-us/articles/360059254971";
}  // namespace

void LegacyTLSUI::HandleCommand(SecurityInterstitialCommand command) {
  if (command == CMD_OPEN_HELP_CENTER) {
    controller_->metrics_helper()->RecordUserInteraction(
        security_interstitials::MetricsHelper::SHOW_LEARN_MORE);

    // Redirect to the support article.
    controller_->OpenUrlInNewForegroundTab(
        controller_->GetBaseHelpCenterUrl().Resolve(kHelpArticle));
    return;
  }

  HandleCommand_ChromiumImpl(command);
}

}  // namespace security_interstitials
