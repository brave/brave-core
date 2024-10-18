// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_TRANSLATE_WEB_VIEW_TRANSLATE_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_TRANSLATE_WEB_VIEW_TRANSLATE_CLIENT_H_

#include <memory>

#include "components/translate/core/browser/translate_client.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/web_state.h"
#import "ios/web_view/internal/translate/cwv_translation_controller_internal.h"

namespace ios_web_view {

class WebViewTranslateClient : public translate::TranslateClient {
 public:
  static std::unique_ptr<WebViewTranslateClient> Create(
      web::BrowserState* browser_state,
      web::WebState* web_state);

  void set_translation_controller(CWVTranslationController* controller) {}
  void TranslatePage(const std::string& source_lang,
                     const std::string& target_lang,
                     bool triggered_from_menu) {}
  void RevertTranslation() {}
  bool RequestTranslationOffer() { return false; }
};

}  // namespace ios_web_view

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_VIEW_INTERNAL_TRANSLATE_WEB_VIEW_TRANSLATE_CLIENT_H_
