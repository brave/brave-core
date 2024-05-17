/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DEPRECATED_CHANNEL_NAME_CHANNEL_NAME_H_
#define BRAVE_COMPONENTS_DEPRECATED_CHANNEL_NAME_CHANNEL_NAME_H_

#include <string>

#include "base/version_info/channel.h"

namespace deprecated_channel_name {
// DEPRECATED: Use `version_info::GetChannelString(chrome::GetChannel())`
// instead.
std::string GetChannelName(version_info::Channel channel);

}  // namespace deprecated_channel_name

#endif  // BRAVE_COMPONENTS_DEPRECATED_CHANNEL_NAME_CHANNEL_NAME_H_
