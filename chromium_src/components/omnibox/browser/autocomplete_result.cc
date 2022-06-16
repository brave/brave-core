/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "src/components/omnibox/browser/autocomplete_result.cc"

// Move match to |index|.
void AutocompleteResult::ReorderMatch(const ACMatches::iterator& it,
                                      int index) {
  DCHECK_LT(index, static_cast<int>(size()));
  const bool move_to_end = (index < 0);
  const auto pos = move_to_end ? std::prev(end()) : std::next(begin(), index);
  // If |match| is already at the desired position, there's nothing to do.
  if (it == pos)
    return;

  // Rotate |match| to be at the desired position.
  if (pos < it)
    std::rotate(pos, it, std::next(it));
  else
    std::rotate(it, std::next(it), std::next(pos));
}

void AutocompleteResult::RemoveMatch(const ACMatches::iterator& it) {
  matches_.erase(it);
}
