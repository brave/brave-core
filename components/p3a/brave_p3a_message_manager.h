/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/strings/string_piece_forward.h"
#include "base/timer/timer.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/brave_p3a_metric_log_store.h"
#include "brave/components/p3a/p3a_message.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace brave {

struct BraveP3AConfig;

class BraveP3ANewUploader;
class BraveP3AStarLogStore;
class BraveP3ARotationScheduler;
class BraveP3AScheduler;
class BraveP3AStar;
class BraveP3AUploader;

struct RandomnessServerInfo;

class BraveP3AMessageManager : public BraveP3AMetricLogStore::Delegate {
 public:
  BraveP3AMessageManager(PrefService* local_state,
                         BraveP3AConfig* config,
                         std::string channel,
                         std::string week_of_install);
  ~BraveP3AMessageManager() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void Init(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  void UpdateMetricValue(base::StringPiece histogram_name, size_t bucket);

  void RemoveMetricValue(base::StringPiece histogram_name);

 private:
  void StartScheduledUpload(bool is_star);
  void StartScheduledStarPrep();

  void OnLogUploadComplete(bool is_ok, int response_code, bool is_star);

  void OnNewStarMessage(std::string histogram_name,
                        uint8_t epoch,
                        std::unique_ptr<std::string> serialized_message);

  void OnRandomnessServerInfoReady(RandomnessServerInfo* server_info);

  // Restart the uploading process (i.e. mark all values as unsent).
  void DoJsonRotation();

  void DoStarRotation();

  std::string SerializeLog(base::StringPiece histogram_name,
                           const uint64_t value,
                           bool is_star) override;

  PrefService* local_state_ = nullptr;

  MessageMetainfo message_meta_;

  BraveP3AConfig* config_;

  std::unique_ptr<BraveP3AMetricLogStore> json_log_store_;
  std::unique_ptr<BraveP3AMetricLogStore> star_prep_log_store_;
  std::unique_ptr<BraveP3AStarLogStore> star_send_log_store_;

  // See `brave_p3a_new_uploader.h`
  std::unique_ptr<BraveP3ANewUploader> new_uploader_;
  std::unique_ptr<BraveP3AScheduler> json_upload_scheduler_;
  std::unique_ptr<BraveP3AScheduler> star_prep_scheduler_;
  std::unique_ptr<BraveP3AScheduler> star_upload_scheduler_;

  std::unique_ptr<BraveP3AStar> star_manager_;

  std::unique_ptr<BraveP3ARotationScheduler> rotation_scheduler_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_H_
