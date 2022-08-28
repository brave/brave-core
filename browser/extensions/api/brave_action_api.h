// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_ACTION_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_ACTION_API_H_

#include <string>

#include "base/observer_list.h"
#include "base/types/expected.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_function.h"
#include "extensions/common/extension.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class Browser;

namespace extensions {
class BraveActionAPI : public KeyedService {
 public:
  class Observer {
   public:
    Observer();
    virtual void OnBraveActionShouldTrigger(
        const std::string& extension_id,
        const absl::optional<std::string>& ui_relative_path) = 0;

   protected:
    virtual ~Observer();
  };

  static BraveActionAPI* Get(Browser* context);
  static base::expected<bool, std::string> ShowActionUI(
      ExtensionFunction* extension_function,
      const std::string& extension_id,
      absl::optional<int> window_id,
      absl::optional<std::string> ui_relative_path);
  static base::expected<bool, std::string> ShowActionUI(
      Browser* browser,
      const std::string& extension_id,
      absl::optional<std::string> ui_relative_path);
  BraveActionAPI();
  BraveActionAPI(const BraveActionAPI&) = delete;
  BraveActionAPI& operator=(const BraveActionAPI&) = delete;
  ~BraveActionAPI() override;

  // Add or remove observers.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  bool NotifyObservers(const std::string& extension_id,
                       absl::optional<std::string> ui_relative_path_param);

 private:
  base::ObserverList<Observer>::Unchecked observers_;
};
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_ACTION_API_H_
