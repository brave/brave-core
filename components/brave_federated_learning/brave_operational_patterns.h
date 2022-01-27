/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

//-------------------------------------------------------------------------------
// |BraveOperationalPatterns| is a class for handling the collection of
// operational patterns, which are are anonymous, minimal representations of how
// users engage with the browser over a collection period. A collection period
// is divided into collection slots (i.e. 30m intervals). Two timers are
// instatiated at startup:
// 1. |collection_slot_periodic_timer_| fires every |collection_slot_size_|/2
// minutes (at most twice per collection slot) and starts the next timer.
// 2. |simulate_local_training_step_timer_| fires a set number of minutes after
// the |collection_slot_periodic_timer_|. When this timer fires, a ping is sent
// to the server.
//
// For more information see
// https://github.com/brave/brave-browser/wiki/Operational-Patterns
//
//-------------------------------------------------------------------------------

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PATTERNS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PATTERNS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

class PrefRegistrySimple;
class PrefService;

namespace net {
class HttpResponseHeaders;
}

namespace network {

class SharedURLLoaderFactory;
class SimpleURLLoader;

}  // namespace network

namespace brave {

class BraveOperationalPatterns final {
 public:
  BraveOperationalPatterns(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BraveOperationalPatterns();
  BraveOperationalPatterns(const BraveOperationalPatterns&) = delete;
  BraveOperationalPatterns& operator=(const BraveOperationalPatterns&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void Start();
  void Stop();

 private:
  void OnCollectionSlotStartTimerFired();
  void OnSimulateLocalTrainingStepTimerFired();
  void OnUploadComplete(scoped_refptr<net::HttpResponseHeaders> headers);

  void SendCollectionSlot();

  void SavePrefs();
  void LoadPrefs();

  std::string BuildPayload() const;
  int GetCurrentCollectionSlot() const;

  void MaybeResetCollectionId();

  raw_ptr<PrefService> pref_service_ = nullptr;
  std::unique_ptr<base::RepeatingTimer> collection_slot_periodic_timer_;
  std::unique_ptr<base::RetainingOneShotTimer>
      simulate_local_training_step_timer_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  base::Time collection_id_expiration_time_;
  int current_collected_slot_ = 0;
  int last_checked_slot_ = 0;
  std::string collection_id_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_LEARNING_BRAVE_OPERATIONAL_PATTERNS_H_
