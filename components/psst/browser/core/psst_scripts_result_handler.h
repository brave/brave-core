/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SCRIPTS_RESULT_HANDLER_H_
#define BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SCRIPTS_RESULT_HANDLER_H_

#include "base/component_export.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/psst/browser/core/psst_dialog_delegate.h"

namespace psst {

class PsstOperationContext;

class COMPONENT_EXPORT(PSST_BROWSER_CORE) PsstScriptsHandler {
 public:
  explicit PsstScriptsHandler(PsstOperationContext* context, PsstDialogDelegate* delegate);
  ~PsstScriptsHandler();

void ProcessUserScriptResult();

private:
raw_ptr<PsstOperationContext> context_;
raw_ptr<PsstDialogDelegate> delegate_;
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_BROWSER_CORE_PSST_SCRIPTS_RESULT_HANDLER_H_