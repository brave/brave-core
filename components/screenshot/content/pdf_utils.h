/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SCREENSHOT_CONTENT_PDF_UTILS_H_
#define BRAVE_COMPONENTS_SCREENSHOT_CONTENT_PDF_UTILS_H_

namespace content {
class WebContents;
}  // namespace content

namespace screenshot {

bool IsPdf(content::WebContents* web_contents);

}  // namespace screenshot

#endif  // BRAVE_COMPONENTS_SCREENSHOT_CONTENT_PDF_UTILS_H_
