/* Copyright (c) 2021 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/test/base/testing_brave_browser_process.h"

// static
TestingBraveBrowserProcess* TestingBraveBrowserProcess::GetGlobal() {
  return static_cast<TestingBraveBrowserProcess*>(g_brave_browser_process);
}

// static
void TestingBraveBrowserProcess::CreateInstance() {
  DCHECK(!g_brave_browser_process);
  TestingBraveBrowserProcess* process = new TestingBraveBrowserProcess;
  g_brave_browser_process = process;
}

// static
void TestingBraveBrowserProcess::DeleteInstance() {
  g_brave_browser_process = nullptr;
}

TestingBraveBrowserProcess::TestingBraveBrowserProcess() {}

void TestingBraveBrowserProcess::StartBraveServices() {}

brave_shields::AdBlockService* TestingBraveBrowserProcess::ad_block_service() {
  DCHECK(ad_block_service_);
  return ad_block_service_.get();
}

void TestingBraveBrowserProcess::SetAdBlockService(
    std::unique_ptr<brave_shields::AdBlockService> service) {
  ad_block_service_ = std::move(service);
}
