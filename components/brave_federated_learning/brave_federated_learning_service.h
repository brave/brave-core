/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;
class PrefRegistrySimple;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave {

class BraveOperationalPatterns;

// In the absence of user data collection, Brave is unable to support learning
// and decisioning systems for tasks such as private ad matching or private news
// recommendation in the traditional centralised paradigm. We aim to build a
// private federated learning platform, to unlock the value of user generated
// data in a secure and privacy preserving manner. This component provides the
// necessary functionality to adopter applications.
class BraveFederatedLearningService : public KeyedService {
 public:
  BraveFederatedLearningService(
      PrefService* prefs,
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveFederatedLearningService() override;

  BraveFederatedLearningService(const BraveFederatedLearningService&) = delete;
  BraveFederatedLearningService& operator=(
      const BraveFederatedLearningService&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void Start();

 private:
  void InitPrefChangeRegistrar();
  void OnPreferenceChanged(const std::string& key);

  bool ShouldStartOperationalPatterns();
  bool IsP3AEnabled();
  bool IsOperationalPatternsEnabled();

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<PrefService> local_state_ = nullptr;
  PrefChangeRegistrar local_state_change_registrar_;
  std::unique_ptr<BraveOperationalPatterns> operational_patterns_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_
