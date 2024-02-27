// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/test/engine_test_observer.h"

EngineTestObserver::EngineTestObserver(brave_shields::AdBlockEngine* engine)
    : engine_(engine) {
  engine_->AddObserverForTest(this);
}

EngineTestObserver::~EngineTestObserver() {
  engine_->RemoveObserverForTest();
}

// Blocks until the engine is updated
void EngineTestObserver::Wait() {
  run_loop_.Run();
}

void EngineTestObserver::OnEngineUpdated() {
  run_loop_.Quit();
}
