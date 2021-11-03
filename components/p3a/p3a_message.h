/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_
#define BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_

#include <cstdint>
#include <string>
#include "base/time/time.h"

namespace brave_pyxis {
class RawP3AValue;
}

namespace brave {

struct MessageMetainfo {
  MessageMetainfo();
  ~MessageMetainfo();

  std::string platform;
  std::string version;
  std::string channel;
  base::Time date_of_install;
  base::Time date_of_survey;
  int woi;  // Week of install. Remove this occasionally and extract from above.
  int wos;  // Week of survey.
  std::string country_code;
  std::string refcode;
};

void GenerateP3AMessage(uint64_t metric_hash,
                        uint64_t metric_value,
                        const MessageMetainfo& meta,
                        brave_pyxis::RawP3AValue* p3a_message);

// Ensures that country/refcode represent the big enough cohort that will not
// let anybody identify the sender.
void MaybeStripRefcodeAndCountry(MessageMetainfo* meta);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_P3A_MESSAGE_H_
