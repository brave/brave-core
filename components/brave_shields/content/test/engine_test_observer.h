// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_ENGINE_TEST_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_ENGINE_TEST_OBSERVER_H_

#include <memory>

#include "base/memory/weak_ptr.h"
namespace base {
class RunLoop;
}

namespace brave_shields {
class AdBlockService;
}

// A test observer that allows blocking waits for an AdBlockEngine to be
// updated with new rules.
class EngineTestObserver {
 public:
  // Constructs an EngineTestObserver observing the selected engine
  // asynchronously via AdBlockService task-runner APIs.
  explicit EngineTestObserver(
      bool is_default_engine,
      brave_shields::AdBlockService* ad_block_service = nullptr);
  ~EngineTestObserver();

  // Blocks until the engine is updated
  void Wait();

 private:
  void OnEngineUpdated();

  std::unique_ptr<base::RunLoop> run_loop_;

  base::WeakPtrFactory<EngineTestObserver> weak_factory_{this};
};

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_ENGINE_TEST_OBSERVER_H_
