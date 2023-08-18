/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_LIST_OPT_IN_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_LIST_OPT_IN_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/common/cookie_list_opt_in.mojom-forward.h"
#include "brave/components/brave_shields/common/cookie_list_opt_in.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace brave_shields {
class AdBlockService;

// This class is not thread-safe and should have single owner
class CookieListOptInService : public KeyedService,
                               public mojom::CookieListOptInPageAndroidHandler {
 public:
  CookieListOptInService(AdBlockService* ad_block_service,
                         PrefService* local_state);
  ~CookieListOptInService() override;

  mojo::PendingRemote<mojom::CookieListOptInPageAndroidHandler> MakeRemote();
  void Bind(
      mojo::PendingReceiver<mojom::CookieListOptInPageAndroidHandler> receiver);

  void OnTooltipShown() override;
  void OnTooltipNoClicked() override;
  void OnTooltipYesClicked() override;

 private:
  raw_ptr<AdBlockService> ad_block_service_ = nullptr;
  raw_ptr<PrefService> local_state_ = nullptr;
  mojo::ReceiverSet<mojom::CookieListOptInPageAndroidHandler> receivers_;
  base::WeakPtrFactory<CookieListOptInService> discovery_weak_factory_{this};

  CookieListOptInService(const CookieListOptInService&) = delete;
  CookieListOptInService& operator=(const CookieListOptInService&) = delete;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_COOKIE_LIST_OPT_IN_SERVICE_H_
