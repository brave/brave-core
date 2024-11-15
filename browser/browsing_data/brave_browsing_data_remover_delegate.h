/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BROWSING_DATA_BRAVE_BROWSING_DATA_REMOVER_DELEGATE_H_
#define BRAVE_BROWSER_BROWSING_DATA_BRAVE_BROWSING_DATA_REMOVER_DELEGATE_H_

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
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
  ~BraveBrowsingDataRemoverDelegate() override;

  BraveBrowsingDataRemoverDelegate(const BraveBrowsingDataRemoverDelegate&) =
      delete;
  BraveBrowsingDataRemoverDelegate operator=(
      const BraveBrowsingDataRemoverDelegate&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveBrowsingDataRemoverDelegateTest,
                           ShieldsSettingsClearTest);

  // ChromeBrowsingDataRemoverDelegate overrides:
  void RemoveEmbedderData(const base::Time& delete_begin,
                          const base::Time& delete_end,
                          uint64_t remove_mask,
                          content::BrowsingDataFilterBuilder* filter_builder,
                          uint64_t origin_type_mask,
                          base::OnceCallback<void(uint64_t)> callback) override;

  void ClearShieldsSettings(base::Time begin_time, base::Time end_time);

  raw_ptr<Profile> profile_ = nullptr;
  base::WeakPtrFactory<BraveBrowsingDataRemoverDelegate> weak_ptr_factory_{
      this};
};

#endif  // BRAVE_BROWSER_BROWSING_DATA_BRAVE_BROWSING_DATA_REMOVER_DELEGATE_H_
