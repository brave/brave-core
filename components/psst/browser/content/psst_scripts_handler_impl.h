// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_IMPL_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_IMPL_H_

#include <string>

#include "brave/components/psst/browser/content/psst_tab_web_contents_observer.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace psst {

class PsstScriptsHandlerImpl
    : public PsstTabWebContentsObserver::ScriptsHandler {
 public:
  explicit PsstScriptsHandlerImpl(content::WebContents* web_contents,
                                  const int32_t world_id);
  ~PsstScriptsHandlerImpl() override;

  // PsstScriptsHandler overrides
  void InsertScriptInPage(
      const std::string& script,
      PsstTabWebContentsObserver::InsertScriptInPageCallback cb) override;

 private:
  raw_ptr<content::WebContents> web_contents_;
  const int32_t world_id_;

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CONTENT_PSST_SCRIPTS_HANDLER_IMPL_H_
