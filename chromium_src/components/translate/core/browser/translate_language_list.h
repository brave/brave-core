// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_

#define TranslateLanguageList TranslateLanguageList_ChromiumImpl
#define SetResourceRequestsAllowed virtual SetResourceRequestsAllowed
#include "src/components/translate/core/browser/translate_language_list.h"  // IWYU pragma: export
#undef SetResourceRequestsAllowed
#undef TranslateLanguageList

namespace translate {

class TranslateLanguageList : public TranslateLanguageList_ChromiumImpl {
 public:
  void SetResourceRequestsAllowed(bool allowed) override;
};

}  // namespace translate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_LANGUAGE_LIST_H_
