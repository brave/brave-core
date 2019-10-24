/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_ACTION_VIEW_H_

#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"

namespace gfx {
  class Rect;
}

// Subclasses ToolbarActionViewc so that the notification badge can be painted
// outside the highlight bubble.
class BraveActionView : public ToolbarActionView {
 public:
  BraveActionView(ToolbarActionViewController* view_controller,
                  ToolbarActionView::Delegate* delegate);

  SkPath GetHighlightPath() const;

  DISALLOW_COPY_AND_ASSIGN(BraveActionView);
};

#endif
