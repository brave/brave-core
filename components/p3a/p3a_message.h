/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_
#define BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_

#include <cstdint>
#include <string>

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/p3a/metric_log_type.h"

class PrefService;

namespace brave {

constexpr const char* kP3AMessageStarKeyValueSeparator = "|";
constexpr const char* kP3AMessageStarLayerSeparator = ";";

class MessageMetainfo {
 public:
  MessageMetainfo();
  ~MessageMetainfo();

  void Init(PrefService* local_state,
            std::string brave_channel,
            std::string week_of_install);

  void Update();

  void MaybeStripCountry();

  std::string platform;
  std::string version;
  std::string channel;
  base::Time date_of_install;
  base::Time date_of_survey;
  int woi;  // Week of install. Remove this occasionally and extract from above.
  std::string country_code;
};

base::Value::Dict GenerateP3AMessageDict(base::StringPiece metric_name,
                                         uint64_t metric_value,
                                         MetricLogType log_type,
                                         const MessageMetainfo& meta,
                                         const std::string& upload_type);

std::string GenerateP3AStarMessage(base::StringPiece metric_name,
                                   uint64_t metric_value,
                                   const MessageMetainfo& meta);

// Ensures that country code represents a big enough cohort that
// no one can identify the sender.
void MaybeStripCountry(MessageMetainfo* meta);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_
