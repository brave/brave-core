// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNEL_MIGRATOR_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNEL_MIGRATOR_H_

#include <string>

#include "components/prefs/pref_service.h"

namespace brave_news {

void MigrateChannels(PrefService& prefs);
std::string GetMigratedChannel(const std::string& channel);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNEL_MIGRATOR_H_
