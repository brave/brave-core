/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_RESULT_HANDLER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_RESULT_HANDLER_H_

#include <memory>

#include "base/component_export.h"
#include "brave/components/psst/browser/core/psst_dialog_delegate.h"
#include "brave/components/psst/common/psst_prefs.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/web_contents.h"

namespace psst {

class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstScriptsHandler {
 public:
  PsstScriptsHandler(const PsstScriptsHandler&) = delete;
  PsstScriptsHandler& operator=(const PsstScriptsHandler&) = delete;

  PsstScriptsHandler();
  virtual ~PsstScriptsHandler();

  virtual void Start() = 0;

  virtual PsstDialogDelegate* GetPsstDialogDelegate() = 0;
};

class COMPONENT_EXPORT(PSST_BROWSER_CONTENT) PsstScriptsHandlerImpl
    : public PsstScriptsHandler {
 public:
  explicit PsstScriptsHandlerImpl(
      std::unique_ptr<PsstDialogDelegate> delegate,
      PrefService* prefs,
      content::WebContents* web_contents,
      const content::RenderFrameHost* render_frame_host,
      const int32_t world_id);
  ~PsstScriptsHandlerImpl() override;

  void Start() override;

  PsstDialogDelegate* GetPsstDialogDelegate() override;

 private:
  std::unique_ptr<PsstDialogDelegate> delegate_;
  const raw_ptr<PrefService> prefs_;

  const content::GlobalRenderFrameHostId render_frame_host_id_;
  raw_ptr<content::WebContents> web_contents_{nullptr};
  const int32_t world_id_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_RESULT_HANDLER_H_
