/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/test_utils.h"

#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"

namespace brave_wallet {

void WaitForTxStateManagerInitialized(TxStateManager* tx_state_manager) {
  base::RunLoop run_loop;
  class TestTxStateManagerObserver : public TxStateManager::Observer {
   public:
    explicit TestTxStateManagerObserver(TxStateManager* tx_state_manager,
                                        base::RunLoop& run_loop)
        : run_loop_(run_loop) {
      observation_.Observe(tx_state_manager);
    }

    void OnInitialized() override { run_loop_.Quit(); }

   private:
    base::ScopedObservation<TxStateManager, TxStateManager::Observer>
        observation_{this};
    base::RunLoop& run_loop_;
  } observer(tx_state_manager, run_loop);
  run_loop.Run();
}

}  // namespace brave_wallet
