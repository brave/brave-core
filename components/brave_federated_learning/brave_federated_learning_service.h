/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;
class PrefRegistrySimple;

namespace brave {

namespace federated_learning {

class DataStoreService;
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
      const base::FilePath brave_federated_learning_path,
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

  bool IsFederatedLearningEnabled();
  bool ShouldStartOperationalPatterns();
  bool IsP3AEnabled();
  bool IsOperationalPatternsEnabled();

  PrefService* prefs_;
  PrefService* local_state_;
  PrefChangeRegistrar local_state_change_registrar_;
  const base::FilePath brave_federated_learning_path_;

  std::unique_ptr<BraveOperationalPatterns> operational_patterns_;
  std::unique_ptr<DataStoreService> data_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace federated_learning

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_FEDERATED_LEARNING_SERVICE_H_
