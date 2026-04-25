/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_UPDATE_RECOMMENDED_MESSAGE_BOX_MAC_H_
#define BRAVE_BROWSER_UI_VIEWS_UPDATE_RECOMMENDED_MESSAGE_BOX_MAC_H_

#include "chrome/browser/ui/views/update_recommended_message_box.h"

class UpdateRecommendedMessageBoxMac : public UpdateRecommendedMessageBox {
 public:
  UpdateRecommendedMessageBoxMac(const UpdateRecommendedMessageBoxMac&) =
      delete;
  UpdateRecommendedMessageBoxMac& operator=(
      const UpdateRecommendedMessageBoxMac&) = delete;

  static void Show(gfx::NativeWindow parent_window);

 private:
  UpdateRecommendedMessageBoxMac() {}
  ~UpdateRecommendedMessageBoxMac() override {}

  // UpdateRecommendedMessageBox overrides:
  bool Accept() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_UPDATE_RECOMMENDED_MESSAGE_BOX_MAC_H_
