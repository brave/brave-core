// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.


#include "brave/browser/extensions/api/brave_action_api.h"

#include <memory>
#include <string>
#include <utility>

#include "base/no_destructor.h"
#include "components/keyed_service/core/dependency_manager.h"
#include "components/keyed_service/core/keyed_service_factory.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/api/tabs/windows_util.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/window_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "extensions/browser/extension_function.h"

namespace {

class BraveActionAPIDependencyManager : public DependencyManager {
 public:
  static BraveActionAPIDependencyManager* GetInstance() {
    static base::NoDestructor<BraveActionAPIDependencyManager> factory;
    return factory.get();
  }
  BraveActionAPIDependencyManager() = default;
  BraveActionAPIDependencyManager(const BraveActionAPIDependencyManager&) =
      delete;
  BraveActionAPIDependencyManager& operator=(
      const BraveActionAPIDependencyManager&) = delete;
  ~BraveActionAPIDependencyManager() override = default;

#ifndef NDEBUG
void DumpContextDependencies(void* context) const override {}
#endif  // NDEBUG
};

class BraveActionAPIFactory : public KeyedServiceFactory {
 public:
  BraveActionAPIFactory() : KeyedServiceFactory("BraveActionAPI",
      BraveActionAPIDependencyManager::GetInstance(), SIMPLE) { }

  BraveActionAPIFactory(const BraveActionAPIFactory&) = delete;
  BraveActionAPIFactory& operator=(const BraveActionAPIFactory&) = delete;

  extensions::BraveActionAPI* GetBraveActionAPI(Browser* context) {
    return static_cast<extensions::BraveActionAPI*>(
          GetServiceForContext(context, true));
  }

 private:
  // KeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      void* context) const final {
    return base::WrapUnique(new extensions::BraveActionAPI());
  }
  bool IsOffTheRecord(void* context) const final {
    return static_cast<Browser*>(context)
        ->profile()->IsOffTheRecord();
  }
  void* GetContextToUse(void* context) const final {
    return context;
  }
  void CreateServiceNow(void* context) final {
    KeyedServiceFactory::GetServiceForContext(context, true);
  }
};

static BraveActionAPIFactory* GetFactoryInstance() {
  static base::NoDestructor<BraveActionAPIFactory> instance;
  return instance.get();
}

}  // namespace

namespace extensions {
//
// BraveActionAPI::Observer
//
BraveActionAPI::Observer::Observer() = default;

BraveActionAPI::Observer::~Observer() = default;

//
// BraveActionAPI
//
// static
BraveActionAPI* BraveActionAPI::Get(Browser* context) {
  return GetFactoryInstance()->GetBraveActionAPI(context);
}

// static
base::expected<bool, std::string> BraveActionAPI::ShowActionUI(
    ExtensionFunction* extension_function,
    const std::string& extension_id,
    absl::optional<int> window_id_param,
    absl::optional<std::string> ui_relative_path_param) {
  // Which browser should we send the action to
  Browser* browser = nullptr;
  // If the windowId is specified, find it. Otherwise get the active
  // window for the profile.
  if (!window_id_param) {
    browser = ChromeExtensionFunctionDetails(extension_function)
        .GetCurrentBrowser();
    if (!browser) {
      return base::unexpected(tabs_constants::kNoCurrentWindowError);
    }
  } else {
    std::string get_browser_error;
    if (!windows_util::GetBrowserFromWindowID(
            extension_function, *window_id_param,
            WindowController::GetAllWindowFilter(), &browser,
            &get_browser_error)) {
      return base::unexpected(get_browser_error);
    }
  }
  return ShowActionUI(browser, extension_id, std::move(ui_relative_path_param));
}

// static
base::expected<bool, std::string> BraveActionAPI::ShowActionUI(
    Browser* browser,
    const std::string& extension_id,
    absl::optional<std::string> ui_relative_path_param) {
  bool did_notify = BraveActionAPI::Get(browser)->NotifyObservers(extension_id,
      std::move(ui_relative_path_param));
  if (!did_notify) {
    return base::unexpected(
        "No toolbar is registered to observe BraveActionUI "
        "calls for this window");
  }
  return true;
}

BraveActionAPI::BraveActionAPI() = default;

BraveActionAPI::~BraveActionAPI() = default;

void BraveActionAPI::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveActionAPI::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool BraveActionAPI::NotifyObservers(
    const std::string& extension_id,
    absl::optional<std::string> ui_relative_path_param) {
  bool did_notify = false;
  for (auto& observer : observers_) {
    observer.OnBraveActionShouldTrigger(extension_id, ui_relative_path_param);
    did_notify = true;
  }
  return did_notify;
}
}  // namespace extensions
