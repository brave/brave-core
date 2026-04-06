/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/tab_test_helper.h"

#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"
#include "url/gurl.h"

namespace brave_ads::test {

TabHelper::TabHelper(AdsClientNotifier& ads_client_notifier)
    : ads_client_notifier_(ads_client_notifier) {}

TabHelper::~TabHelper() = default;

void TabHelper::OpenTab(int32_t tab_id,
                        const std::vector<GURL>& redirect_chain,
                        int http_status_code) {
  CHECK(!redirect_chains_.contains(tab_id)) << "Tab already open";

  redirect_chains_[tab_id] = redirect_chain;

  SelectTab(tab_id);

  NavigateToUrl(tab_id, redirect_chain, http_status_code);
}

void TabHelper::NavigateToUrl(int32_t tab_id,
                              const std::vector<GURL>& redirect_chain,
                              int http_status_code) {
  CHECK(redirect_chains_.contains(tab_id)) << "Tab does not exist";

  redirect_chains_[tab_id] = redirect_chain;

  const bool is_visible = tab_id == visible_tab_id_;

  ads_client_notifier_->NotifyTabDidChange(tab_id, redirect_chain,
                                           /*is_new_navigation=*/true,
                                           /*is_restoring=*/false, is_visible);
  ads_client_notifier_->NotifyTabDidLoad(tab_id, http_status_code);
}

void TabHelper::SelectTab(int32_t tab_id) {
  CHECK(redirect_chains_.contains(tab_id)) << "Tab does not exist";

  if (visible_tab_id_) {
    // Occlude the previously visible tab.
    CHECK_NE(*visible_tab_id_, tab_id) << "Tab already selected";
    CHECK(redirect_chains_.contains(*visible_tab_id_));

    ads_client_notifier_->NotifyTabDidChange(
        *visible_tab_id_, redirect_chains_[*visible_tab_id_],
        /*is_new_navigation=*/false, /*is_restoring=*/false,
        /*is_visible=*/false);
  }
  visible_tab_id_ = tab_id;

  ads_client_notifier_->NotifyTabDidChange(tab_id, redirect_chains_[tab_id],
                                           /*is_new_navigation=*/false,
                                           /*is_restoring=*/false,
                                           /*is_visible=*/true);
}

void TabHelper::CloseTab(int32_t tab_id) {
  CHECK(redirect_chains_.contains(tab_id)) << "Tab does not exist";

  ads_client_notifier_->NotifyDidCloseTab(tab_id);

  redirect_chains_.erase(tab_id);

  const bool should_select_last_tab =
      tab_id == visible_tab_id_ && !redirect_chains_.empty();
  visible_tab_id_.reset();

  if (should_select_last_tab) {
    SelectLastTab();
  }
}

///////////////////////////////////////////////////////////////////////////////

void TabHelper::SelectLastTab() {
  CHECK(!redirect_chains_.empty()) << "No tabs";

  const auto iter = redirect_chains_.crbegin();
  const auto [tab_id, _] = *iter;
  SelectTab(tab_id);
}

}  // namespace brave_ads::test
