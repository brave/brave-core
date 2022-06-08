/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_H_

#include <memory>
#include <string>

#include "base/strings/string_piece_forward.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/p3a/brave_p3a_log_store.h"
#include "brave/components/p3a/brave_p3a_message_manager_config.h"
#include "brave/components/p3a/p3a_message.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}

namespace brave {

class BraveP3AScheduler;
class BraveP3AUploader;
class BraveP3ANewUploader;
class BraveP3AStarManager;

class BraveP3AMessageManager : public BraveP3ALogStore::Delegate {
 public:
  BraveP3AMessageManager(PrefService* local_state,
                         std::string channel,
                         std::string week_of_install);
  ~BraveP3AMessageManager() override;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void Init(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  void UpdateMetricValue(base::StringPiece histogram_name, size_t bucket);

  void RemoveMetricValue(base::StringPiece histogram_name);

 private:
  void InitMessageMeta(std::string channel, std::string week_of_install);

  // Updates things that change over time: week of survey, etc.
  void UpdateMessageMeta();

  void StartScheduledUpload();

  void OnLogUploadComplete(int response_code, int error_code, bool was_https);

  void OnStarMessageCreated(const char* histogram_name,
                            uint8_t epoch,
                            std::string serialized_message);

  // Restart the uploading process (i.e. mark all values as unsent).
  void DoRotation();

  void UpdateRotationTimer();

  std::string SerializeLog(base::StringPiece histogram_name,
                           const uint64_t value) override;

  PrefService* local_state_ = nullptr;

  MessageMetainfo message_meta_;

  MessageManagerConfig config_;

  // Components:
  std::unique_ptr<BraveP3ALogStore> log_store_;
  // See `brave_p3a_new_uploader.h`
  std::unique_ptr<BraveP3ANewUploader> new_uploader_;
  std::unique_ptr<BraveP3AScheduler> upload_scheduler_;

  std::unique_ptr<BraveP3AStarManager> star_manager_;

  // Once fired we restart the overall uploading process.
  base::WallClockTimer rotation_timer_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_H_
