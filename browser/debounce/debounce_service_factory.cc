/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/debounce/debounce_service_factory.h"

#include <memory>
#include <string>

#include "base/memory/singleton.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/debounce/browser/debounce_service.h"
#include "brave/components/debounce/browser/debounce_service_impl.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/keyed_service.h"

namespace debounce {

// static
DebounceServiceFactory* DebounceServiceFactory::GetInstance() {
  return base::Singleton<DebounceServiceFactory>::get();
}

DebounceService* DebounceServiceFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<DebounceService*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

DebounceServiceFactory::DebounceServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "DebounceService",
          BrowserContextDependencyManager::GetInstance()) {}

DebounceServiceFactory::~DebounceServiceFactory() = default;

KeyedService* DebounceServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  debounce::DebounceDownloadService* download_service = nullptr;
  // Brave browser process may be null if we are being created within a unit
  // test.
  if (g_brave_browser_process)
    download_service = g_brave_browser_process->debounce_download_service();
  std::unique_ptr<DebounceServiceImpl> debounce_service(
      new DebounceServiceImpl(download_service));
  return debounce_service.release();
}

bool DebounceServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

}  // namespace debounce
