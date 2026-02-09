// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"

#include <ios/web_view/internal/autofill/cwv_autofill_data_manager.mm>

// Helper category to expose the underlying `PersonalDataManager` so it may be
// used in //brave/ios/web_view targets to expose functionality missing from
// CWVAutofillDataManager
@implementation CWVAutofillDataManager (Internal)

- (autofill::PersonalDataManager*)personalDataManager {
  return _personalDataManager;
}

@end

#pragma clang diagnostic pop
