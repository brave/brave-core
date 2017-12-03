/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_

#include <stdint.h>
#include <string>

#include "components/content_settings/core/common/content_settings_types.h"
#include "url/gurl.h"


namespace content {
class ResourceContext;
}

namespace net {
class URLRequest;
}

namespace brave_shields {

bool IsAllowContentSetting(content::ResourceContext* resource_context,
    GURL tab_origin, ContentSettingsType setting_type);

void GetRenderFrameIdAndProcessId(net::URLRequest* request,
    int* render_frame_id,
    int* render_process_id);

void DispatchBlockedEvent(const std::string &block_type,
    net::URLRequest* request);

int GetTabId(net::URLRequest* request);
bool GetTabOrigin(net::URLRequest* request, GURL *url);

bool GetUrlForTabId(int tab_id, GURL* url);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_DAT_FILE_UTIL_
