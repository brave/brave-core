/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BROWSING_DATA_BRAVE_BROWSING_DATA_REMOVER_DELEGATE_H_
#define BRAVE_BROWSER_BROWSING_DATA_BRAVE_BROWSING_DATA_REMOVER_DELEGATE_H_

#include "base/time/time.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"

namespace content_settings {
class BravePrefProvider;
}  // namespace content_settings

class Profile;

class BraveBrowsingDataRemoverDelegate
    : public ChromeBrowsingDataRemoverDelegate {
 public:
  explicit BraveBrowsingDataRemoverDelegate(
      content::BrowserContext* browser_context);
  ~BraveBrowsingDataRemoverDelegate() override = default;

  BraveBrowsingDataRemoverDelegate(
      const BraveBrowsingDataRemoverDelegate&) = delete;
  BraveBrowsingDataRemoverDelegate operator=(
      const BraveBrowsingDataRemoverDelegate&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveBrowsingDataRemoverDelegateTest,
                           ShieldsSettingsClearTest);

  // ChromeBrowsingDataRemoverDelegate overrides:
  void RemoveEmbedderData(const base::Time& delete_begin,
                          const base::Time& delete_end,
                          int remove_mask,
                          content::BrowsingDataFilterBuilder* filter_builder,
                          int origin_type_mask,
                          base::OnceClosure callback) override;

  void ClearShieldsSettings(base::Time begin_time, base::Time end_time);

  Profile* profile_;
};

#endif  // BRAVE_BROWSER_BROWSING_DATA_BRAVE_BROWSING_DATA_REMOVER_DELEGATE_H_
