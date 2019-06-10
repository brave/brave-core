/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_TAB_HELPERS_H_
#define BRAVE_BROWSER_BRAVE_TAB_HELPERS_H_

namespace content {
class WebContents;
}

namespace brave {

void AttachTabHelpers(content::WebContents* web_contents);

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_TAB_HELPERS_H_
