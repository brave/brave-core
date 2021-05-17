/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_PREF_STORE_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_PREF_STORE_H_

#include "base/values.h"
#include "extensions/browser/extension_pref_store.h"

class BraveExtensionPrefStore : public ExtensionPrefStore {
 public:
  BraveExtensionPrefStore(ExtensionPrefValueMap* extension_pref_value_map,
                          bool incognito_pref_store);

  // ExtensionPrefStore overrides:
  void OnPrefValueChanged(const std::string& key) override;
  bool GetValue(const std::string& key,
                const base::Value** value) const override;

 protected:
  ~BraveExtensionPrefStore() override;

 private:
  ExtensionPrefValueMap* extension_pref_value_map_;  // Weak pointer.
  base::Value default_extension_search_provider_;
  bool incognito_pref_store_;
};

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_PREF_STORE_H_
