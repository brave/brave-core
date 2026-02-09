// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_DATA_MANAGER_INTERNAL_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_DATA_MANAGER_INTERNAL_H_

#include <ios/web_view/internal/autofill/cwv_autofill_data_manager_internal.h>  // IWYU pragma: export

// Helper category to expose the underlying `PersonalDataManager` so it may be
// used in //brave/ios/web_view targets to expose functionality missing from
// CWVAutofillDataManager
@interface CWVAutofillDataManager (Internal)

- (autofill::PersonalDataManager*)personalDataManager;

@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_DATA_MANAGER_INTERNAL_H_
