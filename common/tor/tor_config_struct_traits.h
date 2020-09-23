/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_TOR_TOR_CONFIG_STRUCT_TRAITS_H_
#define BRAVE_COMMON_TOR_TOR_CONFIG_STRUCT_TRAITS_H_

#include <string>

#include "brave/common/tor/tor_common.h"
#include "brave/components/services/tor/public/interfaces/tor.mojom-shared.h"
#include "ipc/ipc_message_utils.h"

namespace mojo {

template <>
struct StructTraits<tor::mojom::TorConfigDataView,
                    tor::TorConfig> {
  static const base::FilePath& binary_path(const tor::TorConfig& config) {
    return config.binary_path();
  }

  static const base::FilePath& tor_data_path(const tor::TorConfig& config) {
    return config.tor_data_path();
  }

  static const base::FilePath& tor_watch_path(const tor::TorConfig& config) {
    return config.tor_watch_path();
  }

  static bool Read(tor::mojom::TorConfigDataView in,
                   tor::TorConfig* out);
};

}  // namespace mojo

#endif  // BRAVE_COMMON_TOR_TOR_CONFIG_STRUCT_TRAITS_H_
