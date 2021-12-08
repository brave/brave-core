/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_UPDATE_CHECKER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_UPDATE_CHECKER_H_

#include "src/components/update_client/update_checker.h"

#include <deque>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "components/update_client/component.h"
#include "components/update_client/configurator.h"
#include "components/update_client/persisted_data.h"
#include "components/update_client/update_client_errors.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace update_client {

// SequentialUpdateChecker delegates to UpdateChecker to perform a separate
// update request for each component, instead of one request for all components.
// We do for the following reason:
// Google's ToS do not allow distributing all components. In particular, the
// Widevine plugin must be fetched from Google servers. Brave's update server
// for components handles this as follows: When an update for a Google
// component is requested, the server responds with a HTTP redirect to
// Google's server. The problem is that this only works for update requests
// for single components. But Chromium's default implementation sends a list of
// components in one request, which in Brave's case is a mix of Google and Brave
// components. To solve this, we overwrite Chromium's implementation to perform
// separate update requests instead.
class SequentialUpdateChecker : public UpdateChecker {
 public:
  static std::unique_ptr<UpdateChecker> Create(
      scoped_refptr<Configurator> config,
      PersistedData* persistent);

  void CheckForUpdates(
      const std::string& session_id,
      const std::vector<std::string>& ids_checked,
      const IdToComponentPtrMap& components,
      const base::flat_map<std::string, std::string>& additional_attributes,
      UpdateCheckCallback update_check_callback) override;

  // Needs to be public so std::make_unique(...) works in Create(...).
  SequentialUpdateChecker(scoped_refptr<Configurator> config,
                          PersistedData* metadata);
  SequentialUpdateChecker(const SequentialUpdateChecker&) = delete;
  SequentialUpdateChecker& operator=(const SequentialUpdateChecker&) = delete;
  ~SequentialUpdateChecker() override;

 private:
  void CheckNext();
  void UpdateResultAvailable(
      const absl::optional<ProtocolParser::Results>& results,
      ErrorCategory error_category,
      int error,
      int retry_after_sec);

  base::ThreadChecker thread_checker_;

  const scoped_refptr<Configurator> config_;
  PersistedData* metadata_ = nullptr;

  // Store the parameters to CheckForUpdates(...), so we can pass them multiple
  // times to the original UpdateChecker implementation.
  std::string session_id_;
  // Needs to be a pointer because the values in IdToComponentPtrMap are of
  // type std::unique_ptr, which we can't copy. Furthermore, it is okay to keep
  // this pointer because IdToComponentPtrMap resides in in UpdateContext, which
  // outlives this class.
  const IdToComponentPtrMap* components_;
  base::flat_map<std::string, std::string> additional_attributes_;
  UpdateCheckCallback update_check_callback_;

  std::deque<std::string> remaining_ids_;

  // The currently running update_checker_. We keep a smart pointer to it to
  // keep it alive while this particular sequential update check takes place.
  std::unique_ptr<UpdateChecker> update_checker_;
  // Aggregates results from all sequential update requests.
  ProtocolParser::Results results_;
};

}  // namespace update_client

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_UPDATE_CLIENT_UPDATE_CHECKER_H_
