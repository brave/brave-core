#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNEL_MIGRATOR_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNEL_MIGRATOR_H_

#include <string>
#include <string_view>

#include "components/prefs/pref_service.h"

namespace brave_news {

void MigrateChannels(PrefService& prefs);
std::string GetChannelName(const std::string& channel);

}

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_CHANNEL_MIGRATOR_H_
