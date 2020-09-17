//
//  browser_state_dependency_manager.cc
//
//  Created by brandon on 2020-05-08.
//

#include "brave/vendor/brave-ios/components/keyed_service/browser_state_dependency_manager.h"
#include "brave/vendor/brave-ios/components/browser_state/web_browser_state.h"

#include "base/memory/singleton.h"
#include "base/trace_event/trace_event.h"

namespace brave {
// static
BrowserStateDependencyManager* BrowserStateDependencyManager::GetInstance() {
  return base::Singleton<BrowserStateDependencyManager>::get();
}

void BrowserStateDependencyManager::RegisterBrowserStatePrefsForServices(
    user_prefs::PrefRegistrySyncable* pref_registry) {
  RegisterPrefsForServices(pref_registry);
}

void BrowserStateDependencyManager::CreateBrowserStateServices(
    web::BrowserState* context) {
  DoCreateBrowserStateServices(context, false);
}

void BrowserStateDependencyManager::CreateBrowserStateServicesForTest(
    web::BrowserState* context) {
  DoCreateBrowserStateServices(context, true);
}

void BrowserStateDependencyManager::DestroyBrowserStateServices(
    web::BrowserState* context) {
  DependencyManager::DestroyContextServices(context);
}

void BrowserStateDependencyManager::AssertBrowserStateWasntDestroyed(
    web::BrowserState* context) const {
  DependencyManager::AssertContextWasntDestroyed(context);
}

void BrowserStateDependencyManager::MarkBrowserStateLive(
    web::BrowserState* context) {
  DependencyManager::MarkContextLive(context);
}

BrowserStateDependencyManager::BrowserStateDependencyManager() {
}

BrowserStateDependencyManager::~BrowserStateDependencyManager() {
}

void BrowserStateDependencyManager::DoCreateBrowserStateServices(
    web::BrowserState* context,
    bool is_testing_context) {
  TRACE_EVENT0("browser",
               "BrowserStateDependencyManager::DoCreateBrowserStateServices")
  DependencyManager::CreateContextServices(context, is_testing_context);
}

#ifndef NDEBUG
void BrowserStateDependencyManager::DumpContextDependencies(
    void* context) const {}
#endif  // NDEBUG
}
