/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_GEOLOCATION_ACCURACY_HELPER_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_GEOLOCATION_ACCURACY_HELPER_DIALOG_VIEW_H_

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

class PrefService;

namespace views {
class Checkbox;
}  // namespace views

class GeolocationAccuracyHelperDialogView : public views::DialogDelegateView {
 public:
  METADATA_HEADER(GeolocationAccuracyHelperDialogView);

  GeolocationAccuracyHelperDialogView(PrefService* prefs,
                                      base::OnceClosure closing_callback);
  GeolocationAccuracyHelperDialogView(
      const GeolocationAccuracyHelperDialogView&) = delete;
  GeolocationAccuracyHelperDialogView& operator=(
      const GeolocationAccuracyHelperDialogView&) = delete;
  ~GeolocationAccuracyHelperDialogView() override;

 private:
  // views::DialogDelegateView overrides:
  void AddedToWidget() override;
  void OnWidgetInitialized() override;

  void SetupChildViews();
  void OnCheckboxUpdated();
  void OnAccept();
  void OnLearnMoreClicked();

  const raw_ref<PrefService> prefs_;
  raw_ptr<views::Checkbox> dont_show_again_checkbox_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_GEOLOCATION_ACCURACY_HELPER_DIALOG_VIEW_H_
