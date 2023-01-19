/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/build_channel.h"

#include "base/no_destructor.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

mojom::BuildChannelInfo& BuildChannel() {
  static base::NoDestructor<mojom::BuildChannelInfo> build_channel;
  return *build_channel;
}

}  // namespace ads
