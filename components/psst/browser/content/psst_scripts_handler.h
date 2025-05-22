/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_H_

#include <memory>

#include "base/component_export.h"

class PrefService;

namespace content {
class WebContents;
}  // namespace content

namespace psst {

class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstScriptsHandler {
 public:
  PsstScriptsHandler(const PsstScriptsHandler&) = delete;
  PsstScriptsHandler& operator=(const PsstScriptsHandler&) = delete;

  PsstScriptsHandler();
  virtual ~PsstScriptsHandler();

  static std::unique_ptr<PsstScriptsHandler> Create(
      content::WebContents* contents,
      PrefService* prefs,
      const int32_t world_id);

  virtual void Start() = 0;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_H_
