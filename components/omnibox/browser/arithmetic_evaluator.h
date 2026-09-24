/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_ARITHMETIC_EVALUATOR_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_ARITHMETIC_EVALUATOR_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/feature_list.h"

namespace omnibox {

// Kill switch for answering arithmetic locally. Disabling it leaves the
// queries the long-number check withholds from the suggest server with no
// answer, as they were before this existed.
//
// Declared here rather than with the other omnibox features because this
// target is only built when the strict query check is, which is the only
// caller.
BASE_DECLARE_FEATURE(kBraveLocalCalculator);

// Evaluates `text` as an arithmetic expression, returning the answer formatted
// for the user's locale.
//
// Returns nullopt when `text` can't be evaluated. For example, when it isn't
// containing an expression, doesn't parse, divides by zero, overflows int64,
// or would need more precision than can be displayed (e.g. "10/3"). Callers
// must treat that as unsupported and fall back to their default behavior.
//
// The goal here is to handle basic calculator expressions in the omnibox and
// resolve them without making outbound calls to the search suggest API.
std::optional<std::u16string> EvaluateArithmeticExpression(
    std::u16string_view text);

}  // namespace omnibox

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_ARITHMETIC_EVALUATOR_H_
