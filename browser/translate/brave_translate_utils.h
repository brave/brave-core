/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TRANSLATE_BRAVE_TRANSLATE_UTILS_H_
#define BRAVE_BROWSER_TRANSLATE_BRAVE_TRANSLATE_UTILS_H_

namespace content {
class BrowserContext;
}
namespace translate {

bool IsTranslateExtensionEnabled(content::BrowserContext* context);

bool ShouldOfferExtensionInstallation(content::BrowserContext* context);

bool IsInternalTranslationEnabled(content::BrowserContext* context);

}  // namespace translate

#endif  // BRAVE_BROWSER_TRANSLATE_BRAVE_TRANSLATE_UTILS_H_
