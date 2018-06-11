/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_TOOLBAR_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_TOOLBAR_MODEL_DELEGATE_H_

#include "chrome/browser/ui/browser_toolbar_model_delegate.h"

class BraveToolbarModelDelegate : public BrowserToolbarModelDelegate {
 public:
  BraveToolbarModelDelegate(Browser* browser);
  ~BraveToolbarModelDelegate() override;

 private:
  const gfx::VectorIcon* GetVectorIconOverride() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveToolbarModelDelegate);
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_TOOLBAR_MODEL_DELEGATE_H_
