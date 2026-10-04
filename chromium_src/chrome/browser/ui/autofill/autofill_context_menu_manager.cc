// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/autofill/autofill_context_menu_manager.h"

#include <optional>

#include "base/check_op.h"
#include "chrome/app/chrome_command_ids.h"
#include "ui/base/models/menu_model.h"
#include "ui/menus/simple_menu_model.h"

namespace autofill {

namespace {

void RemoveAutofillFeedbackMenuItem(ui::SimpleMenuModel* menu_model) {
  // Remove feedback menu item if present (and the separator that comes after
  // it).
  const std::optional<size_t> feedback_item_index =
      menu_model->GetIndexOfCommandId(IDC_CONTENT_CONTEXT_AUTOFILL_FEEDBACK);
  if (feedback_item_index.has_value()) {
    menu_model->RemoveItemAt(feedback_item_index.value());
    DCHECK_EQ(ui::MenuModel::TYPE_SEPARATOR,
              menu_model->GetTypeAt(feedback_item_index.value()));
    menu_model->RemoveItemAt(feedback_item_index.value());
  }
}

}  // namespace

}  // namespace autofill

#include <chrome/browser/ui/autofill/autofill_context_menu_manager.cc>
