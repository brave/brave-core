/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/browser/core/psst_scripts_result_handler.h"

namespace psst {

PsstScriptsHandler::PsstScriptsHandler(PsstOperationContext* context,
                                       PsstDialogDelegate* delegate)
    : context_(context), delegate_(delegate) {}

PsstScriptsHandler::~PsstScriptsHandler() = default;

void PsstScriptsHandler::ProcessUserScriptResult() {
    
}

}  // namespace psst
