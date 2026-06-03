// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_AD_BLOCK_SERVICE_TEST_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_AD_BLOCK_SERVICE_TEST_OBSERVER_H_

#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"

namespace brave_shields {

// Blocking waiter for AdBlockService filter-list load events. Observes
// AdBlockService::Observer::OnFilterListLoaded which fires on the UI sequence,
// avoiding the cross-sequence race the engine-scoped test observer suffered
// from. Single-use per engine.
class AdBlockServiceTestObserver : public AdBlockService::Observer {
 public:
  explicit AdBlockServiceTestObserver(AdBlockService* service);
  AdBlockServiceTestObserver(const AdBlockServiceTestObserver&) = delete;
  AdBlockServiceTestObserver& operator=(const AdBlockServiceTestObserver&) =
      delete;
  ~AdBlockServiceTestObserver() override;

  // Blocks until OnFilterListLoaded fires for the specified engine. Quit is
  // latched so the event may fire before or after the call; Wait returns in
  // either case.
  void WaitForDefault();
  void WaitForAdditional();

  // Convenience for callers with a dynamic engine choice.
  void Wait(bool is_default_engine);

 private:
  // AdBlockService::Observer:
  void OnFilterListLoaded(bool is_default_engine,
                          AdBlockService::FilterListLoadResult result) override;

  base::RunLoop default_loop_;
  base::RunLoop additional_loop_;
  base::ScopedObservation<AdBlockService, AdBlockService::Observer>
      observation_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_AD_BLOCK_SERVICE_TEST_OBSERVER_H_
