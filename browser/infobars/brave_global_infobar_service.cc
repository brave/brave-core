/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/infobars/brave_global_infobar_service.h"

#include <memory>

#include "base/containers/contains.h"
#include "brave/browser/infobars/brave_ipfs_always_start_infobar_delegate.h"
#include "brave/browser/ui/views/infobars/brave_global_infobar_manager.h"
#include "components/infobars/core/infobar_delegate.h"

BraveGlobalInfobarService::BraveGlobalInfobarService(PrefService* prefs)
    : prefs_(prefs) {
  infobar_managers_[infobars::InfoBarDelegate::InfoBarIdentifier::
                        BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE] =
      std::make_unique<BraveGlobalInfoBarManager>(
          std::make_unique<BraveIPFSAlwaysStartInfoBarDelegateFactory>(prefs));
}

BraveGlobalInfobarService::~BraveGlobalInfobarService() = default;

void BraveGlobalInfobarService::ShowAlwaysStartInfobar() {
  const auto ib_type = infobars::InfoBarDelegate::InfoBarIdentifier::
      BRAVE_IPFS_ALWAYS_START_INFOBAR_DELEGATE;
  const bool is_exists = base::Contains(infobar_managers_, ib_type);
  DCHECK(is_exists);
  if (!is_exists) {
    return;
  }

  infobar_managers_[ib_type]->Show();
}
