/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_

#include <stdint.h>
#include <string>

#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"


namespace net {
class URLRequest;
}

namespace brave_shields {

bool IsAllowContentSettingFromIO(net::URLRequest* request,
    GURL tab_origin, ContentSettingsType setting_type);

void DispatchBlockedEventFromIO(net::URLRequest* request,
    const std::string &block_type);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_
