// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_REWARDS_DATA_BRAVE_REWARDS_DATA_REMOVER_DELEGATE_H_
#define BRAVE_BROWSER_REWARDS_DATA_BRAVE_REWARDS_DATA_REMOVER_DELEGATE_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browsing_data_remover.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"

class Profile;

namespace content {
class BrowserContext;
}

class BraveBrowsingDataRemoverDelegate
    : public ChromeBrowsingDataRemoverDelegate
{
 public:
  enum DataType {
    DATA_TYPE_REWARDS_AUTO_CONTRIBUTE = 1 << 0,
    DATA_TYPE_REWARDS_OTHER = 1 << 1,
    DATA_TYPE_REWARDS_ALL_DATA =
        DATA_TYPE_REWARDS_AUTO_CONTRIBUTE | DATA_TYPE_REWARDS_OTHER,
  };

  explicit BraveBrowsingDataRemoverDelegate(content::BrowserContext* context);

  ~BraveBrowsingDataRemoverDelegate() override;

  void RemoveEmbedderData(
      const base::Time& delete_begin,
      const base::Time& delete_end,
      int remove_mask,
      const content::BrowsingDataFilterBuilder& filter_builder,
      int origin_type_mask,
      base::OnceClosure callback) override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowsingDataRemoverDelegate);
};

#endif  // BRAVE_BROWSER_REWARDS_DATA_BRAVE_REWARDS_DATA_REMOVER_DELEGATE_H_
