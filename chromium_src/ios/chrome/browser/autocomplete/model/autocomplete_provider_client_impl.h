// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_AUTOCOMPLETE_MODEL_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_AUTOCOMPLETE_MODEL_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "build/build_config.h"
#include "components/omnibox/browser/autocomplete_provider_client.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#define GetAccountBookmarkModel                       \
  GetAccountBookmarkModel() override;                 \
  void OpenLeo(const std::u16string& query) override; \
  bool IsLeoProviderEnabled
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#include "src/ios/chrome/browser/autocomplete/model/autocomplete_provider_client_impl.h"  // IWYU pragma: export

#undef GetAccountBookmarkModel

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_AUTOCOMPLETE_MODEL_AUTOCOMPLETE_PROVIDER_CLIENT_IMPL_H_
