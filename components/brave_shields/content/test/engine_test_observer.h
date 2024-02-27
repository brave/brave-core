// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_ENGINE_TEST_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_ENGINE_TEST_OBSERVER_H_

#include "base/run_loop.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"

// A test observer that allows blocking waits for an AdBlockEngine to be
// updated with new rules.
class EngineTestObserver : public brave_shields::AdBlockEngine::TestObserver {
 public:
  // Constructs an EngineTestObserver which will observe the given adblock
  // engine for filter data updates.
  explicit EngineTestObserver(brave_shields::AdBlockEngine* engine);
  ~EngineTestObserver() override;

  EngineTestObserver(const EngineTestObserver& other);
  EngineTestObserver& operator=(const EngineTestObserver& other);

  // Blocks until the engine is updated
  void Wait();

 private:
  void OnEngineUpdated() override;

  base::RunLoop run_loop_;
  raw_ptr<brave_shields::AdBlockEngine> engine_ = nullptr;
};

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_TEST_ENGINE_TEST_OBSERVER_H_
