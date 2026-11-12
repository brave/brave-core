// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
#define CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "ui/base/models/list_selection_model.h"

#if BUILDFLAG(IS_ANDROID)
#error Android doesn't support this tab strip model.
#endif

namespace brave {
class BraveTabStripModelDelegate;
}  // namespace brave

class Profile;

namespace content {
class WebContents;
}

class TabStripModel {
 public:
  explicit TabStripModel(Profile* profile);
};

#endif  // CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
