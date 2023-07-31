/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/url_sanitizer/url_sanitizer_service_factory.h"

#include "base/no_destructor.h"

#include "components/keyed_service/core/keyed_service.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"

#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#include "ios/web/public/browser_state.h"

#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "brave/ios/browser/browser_state/brave_browser_process.h"

namespace brave {

// static
brave::URLSanitizerService* URLSanitizerServiceFactory::GetServiceForState(
    ChromeBrowserState* browser_state) {
  return static_cast<brave::URLSanitizerService*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

// static
URLSanitizerServiceFactory* URLSanitizerServiceFactory::GetInstance() {
  static base::NoDestructor<URLSanitizerServiceFactory> instance;
  return instance.get();
}

URLSanitizerServiceFactory::URLSanitizerServiceFactory()
    : BrowserStateKeyedServiceFactory(
          "URLSanitizerService",
          BrowserStateDependencyManager::GetInstance()) {}

URLSanitizerServiceFactory::~URLSanitizerServiceFactory() = default;

std::unique_ptr<KeyedService>
URLSanitizerServiceFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  std::unique_ptr<brave::URLSanitizerService> service(
      new brave::URLSanitizerService());
  BraveBrowserProcess& browserProcess = BraveBrowserProcess::GetInstance();
  browserProcess.url_sanitizer_component_installer()->AddObserver(
      service.get());
  return service;
}

bool URLSanitizerServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

web::BrowserState* URLSanitizerServiceFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

}  // namespace brave
