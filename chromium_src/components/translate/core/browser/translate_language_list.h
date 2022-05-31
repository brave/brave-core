/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_

namespace translate {
class TranslateLanguageList;
using TranslateLanguageList_BraveImpl = TranslateLanguageList;
}  // namespace translate

#define TranslateLanguageList TranslateLanguageList_ChromiumImpl
#define resource_requests_allowed_ \
  resource_requests_allowed_;      \
  friend TranslateLanguageList_BraveImpl
#include "src/components/translate/core/browser/translate_language_list.h"
#undef resource_requests_allowed_
#undef TranslateLanguageList

namespace translate {

class TranslateLanguageList : public TranslateLanguageList_ChromiumImpl {
 public:
  TranslateLanguageList();
};

}  // namespace translate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_
