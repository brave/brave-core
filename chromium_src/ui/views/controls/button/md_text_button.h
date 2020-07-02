// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_

#include "base/optional.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/focus_ring.h"
#include "ui/views/style/typography.h"

// Insert our own definition for `Create`, and (in concert with the .cc)
// move chromium's definition to `Create_ChromiumImpl`
#define Create Create_ChromiumImpl( \
        ButtonListener* listener, \
        const base::string16& text, \
        int button_context = style::CONTEXT_BUTTON_MD); \
      static std::unique_ptr<MdTextButton> Create

#include "../../../../../../ui/views/controls/button/md_text_button.h"
#undef Create

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_CONTROLS_BUTTON_MD_TEXT_BUTTON_H_
