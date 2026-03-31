// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/test/engine_test_observer.h"

#include "base/run_loop.h"
#include "base/task/bind_post_task.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine.h"
#include "brave/components/brave_shields/content/browser/ad_block_engine_wrapper.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"

EngineTestObserver::EngineTestObserver(
    bool is_default_engine,
    brave_shields::AdBlockService* ad_block_service) {
  if (!ad_block_service) {
    ad_block_service = g_brave_browser_process->ad_block_service();
  }

  auto async_notify_engine_updated =
      base::BindPostTaskToCurrentDefault(base::BindRepeating(
          &EngineTestObserver::OnEngineUpdated, weak_factory_.GetWeakPtr()));
  ad_block_service->AsyncCallOnTaskRunner(base::BindOnce(
      [](bool is_default_engine,
         base::RepeatingClosure async_notify_engine_updated,
         brave_shields::AdBlockEngineWrapper* wrapper) {
        auto& engine = is_default_engine
                           ? wrapper->default_engine_for_testing()
                           : wrapper->additional_filters_engine_for_testing();
        engine.AddOnEngineUpdatedCallbackForTesting(
            std::move(async_notify_engine_updated));
      },
      is_default_engine, std::move(async_notify_engine_updated)));
}

EngineTestObserver::~EngineTestObserver() = default;

void EngineTestObserver::Wait() {
  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop_->Run();
}

void EngineTestObserver::OnEngineUpdated() {
  if (run_loop_) {
    run_loop_->Quit();
    run_loop_ = nullptr;
  }
}
