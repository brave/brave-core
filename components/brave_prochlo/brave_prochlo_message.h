/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_CRYPTO_BRAVE_PROCHLO_MESSAGE_H_
#define BRAVE_COMPONENTS_BRAVE_CRYPTO_BRAVE_PROCHLO_MESSAGE_H_

#include <cstdint>
#include <string>

namespace brave_pyxis {
class PyxisMessage;
}

// TODO(iefremov): prochlo -> pyxis everywhere.
namespace prochlo {

struct MessageMetainfo {
  MessageMetainfo();
  ~MessageMetainfo();

  std::string platform;
  std::string version;
  std::string channel;
  std::string woi;  // Week of install.
};

void GenerateProchloMessage(uint64_t metric_hash,
                            uint64_t metric_value,
                            const MessageMetainfo& meta,
                            brave_pyxis::PyxisMessage* prochlo_message);

}  // namespace prochlo

#endif  // BRAVE_COMPONENTS_BRAVE_CRYPTO_BRAVE_PROCHLO_MESSAGE_H_
