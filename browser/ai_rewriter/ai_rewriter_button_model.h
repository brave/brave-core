// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_H_
#define BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"

namespace ai_rewriter {

class AIRewriterButtonModel {
 public:
  virtual void Show(const gfx::Rect& rect) = 0;
  virtual void Hide() = 0;
};

base::WeakPtr<AIRewriterButtonModel> CreateRewriterButton(
    content::WebContents* contents);

}  // namespace ai_rewriter

#endif  // BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_H_
