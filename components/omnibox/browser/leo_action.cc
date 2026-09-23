/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/leo_action.h"

LeoAction::LeoAction(const std::u16string& query)
    : OmniboxAction({}, {}), query_(query) {}

void LeoAction::Execute(ExecutionContext& context) const {
  context.client_->OpenLeo(query_);
}

LeoAction::~LeoAction() = default;
