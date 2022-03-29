/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// ----------------------------------------------------------------------------
// |OperationalPatterns| handle the collection of anonymous pings with the goal
// of estimating client availability for federated tasks. The collection period
// is divided into descrete slots. The periodic |collection_timer_| will start
// the |mock_task_timer_|, so as to emulate the duration of some federated task.
// If the client is available for the duration of the mock task as indicated by
// the |mock_task_timer_|, the colleciton ping for that slot is sent.
// Pings only contain the minimal amount of information necessary to analyse
// client participation on population level. For more information see
// https://github.com/brave/brave-browser/wiki/Operational-Patterns
// ----------------------------------------------------------------------------

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_H_

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
struct ResourceRequest;

}  // namespace network

namespace brave_federated {

class OperationalPatterns final {
 public:
  OperationalPatterns(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~OperationalPatterns();
  OperationalPatterns(const OperationalPatterns&) = delete;
  OperationalPatterns& operator=(const OperationalPatterns&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void Start();
  void Stop();
  bool IsRunning();

 private:
  void LoadPrefs();
  void SavePrefs();
  void ClearPrefs();

  void StartRepeatingCollectionTimer();
  void OnRepeatingCollectionTimerFired();
  void StopRepeatingCollectionTimer();

  void StartMockTaskTimer();
  void OnMockTaskTimerFired();
  void StopMockTaskTimer();
  void MaybeRestartMockTaskTimer();

  void SendCollectionPing(int slot);
  void OnCollectionPingSend(scoped_refptr<net::HttpResponseHeaders> headers);
  void OnCollectionPingSendSuccess();

  void SendDeletePing();
  void OnDeletePingSend(scoped_refptr<net::HttpResponseHeaders> headers);
  void OnDeletePingSendSuccess();

  void MaybeResetCollectionId();
  void ResetCollectionId();

  raw_ptr<PrefService> pref_service_ = nullptr;  // NOT OWNED
  scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // NOT OWNED

  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  std::unique_ptr<base::RepeatingTimer> collection_timer_;
  std::unique_ptr<base::RetainingOneShotTimer> mock_task_timer_;

  bool is_running_ = false;

  std::string collection_id_;
  base::Time collection_id_expiration_time_;

  int sending_slot_ = -1;
  int last_sent_slot_ = -1;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_OPERATIONAL_PATTERNS_H_
