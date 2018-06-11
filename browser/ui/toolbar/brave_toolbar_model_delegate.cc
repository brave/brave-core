/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_toolbar_model_delegate.h"

#include "brave/common/url_constants.h"
#include "components/toolbar/vector_icons.h"
#include "ui/base/material_design/material_design_controller.h"

BraveToolbarModelDelegate::BraveToolbarModelDelegate(Browser* browser) :
    BrowserToolbarModelDelegate(browser)
{}

BraveToolbarModelDelegate::~BraveToolbarModelDelegate() {}

const gfx::VectorIcon* BraveToolbarModelDelegate::GetVectorIconOverride()
    const {
  GURL url;
  GetURL(&url);

  const bool is_touch_ui =
    ui::MaterialDesignController::IsTouchOptimizedUiEnabled();
  if (url.SchemeIs(kBraveUIScheme)) {
    return is_touch_ui ? &toolbar::kProduct20Icon : &toolbar::kProductIcon;
  }
  return BrowserToolbarModelDelegate::GetVectorIconOverride();
}
