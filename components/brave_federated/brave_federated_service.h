/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_BRAVE_FEDERATED_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_BRAVE_FEDERATED_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ref.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class PrefService;
class PrefRegistrySimple;

namespace brave_federated {

class DataStoreService;
class EligibilityService;
class OperationalPatterns;

// In the absence of user data collection, Brave is unable to support learning
// and decisioning systems for tasks such as private ad matching or private news
// recommendation in the traditional centralised paradigm. We aim to build a
// private federated learning platform, to unlock the value of user generated
// data in a secure and privacy preserving manner. This component provides the
// necessary functionality to adopter applications.
class BraveFederatedService : public KeyedService {
 public:
  BraveFederatedService(
      PrefService* prefs,
      PrefService* local_state,
      const base::FilePath& browser_context_path,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveFederatedService() override;

  BraveFederatedService(const BraveFederatedService&) = delete;
  BraveFederatedService& operator=(const BraveFederatedService&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  DataStoreService* GetDataStoreService() const;

 private:
  void Init(const base::FilePath& browser_context_path);

  void OnPreferenceChanged(const std::string& key);

  bool IsFederatedLearningEnabled();
  bool IsOperationalPatternsEnabled();
  bool IsP3AEnabled();

  bool ShouldStartOperationalPatterns();
  void MaybeStartOrStopOperationalPatterns();
  void MaybeStartOperationalPatterns();
  void MaybeStopOperationalPatterns();

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<PrefService> local_state_ = nullptr;
  PrefChangeRegistrar local_state_change_registrar_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  std::unique_ptr<OperationalPatterns> operational_patterns_;
  std::unique_ptr<DataStoreService> data_store_service_;
  std::unique_ptr<EligibilityService> eligibility_service_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_BRAVE_FEDERATED_SERVICE_H_
