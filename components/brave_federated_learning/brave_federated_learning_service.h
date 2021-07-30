/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "components/prefs/pref_change_registrar.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave {

class BraveOperationalPatterns;

class BraveFederatedLearningService final {
 public:
  BraveFederatedLearningService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveFederatedLearningService();
  BraveFederatedLearningService(const BraveFederatedLearningService&) = delete;
  BraveFederatedLearningService& operator=(
      const BraveFederatedLearningService&) = delete;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  void Start();

 private:
  void InitPrefChangeRegistrar();
  void OnPreferenceChanged(const std::string& key);

  bool IsP3AEnabled();
  bool IsOperationalPatternsEnabled();

  PrefService* local_state_;
  PrefChangeRegistrar local_state_change_registrar_;
  std::unique_ptr<BraveOperationalPatterns> operational_patterns_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_
