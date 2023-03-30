// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/commander/browser/commander_item_model.h"

namespace commander {

CommandItemModel::CommandItemModel(
    const std::u16string& title,
    const std::vector<gfx::Range>& matched_ranges,
    const std::u16string& annotation)
    : title(title), matched_ranges(matched_ranges), annotation(annotation) {}

CommandItemModel::CommandItemModel(const CommandItemModel& other) = default;
CommandItemModel::CommandItemModel(CommandItemModel&& other) = default;

CommandItemModel::~CommandItemModel() = default;

}  // namespace commander
