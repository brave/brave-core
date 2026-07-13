/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SCREENSHOT_CONTENT_PDF_UTILS_H_
#define BRAVE_COMPONENTS_SCREENSHOT_CONTENT_PDF_UTILS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace content {
class WebContents;
}  // namespace content

namespace screenshot {

inline constexpr auto kPrintPreviewRetrievalHosts =
    base::MakeFixedFlatSet<std::string_view>({
        "docs.google.com",
        "watermark.silverchair.com",
    });

bool IsPdf(content::WebContents* web_contents);

}  // namespace screenshot

#endif  // BRAVE_COMPONENTS_SCREENSHOT_CONTENT_PDF_UTILS_H_
