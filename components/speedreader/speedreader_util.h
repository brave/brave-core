/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_

#include <string>
#include <variant>

#include "base/functional/callback_forward.h"
#include "url/gurl.h"

namespace speedreader {

class SpeedreaderService;
class SpeedreaderRewriterService;

enum class DistillationResult : int {
  kNone,
  kSuccess,
  kFail,
};

namespace DistillStates {

using None = std::monostate;

struct DistillReverting;

struct ViewOriginal {
  enum class Reason {
    kNone,            // Original page shown because no action was performed.
    kError,           // Original page shown because distillation was failed.
    kUserAction,      // Original page shown because toggle is clicked.
    kNotDistillable,  // Original page shown because page is not distillable.
  } reason = Reason::kNone;

  bool was_auto_distilled = false;

  ViewOriginal();
  ViewOriginal(Reason reason, bool was_auto_distilled);
  explicit ViewOriginal(const DistillReverting& state);
};

struct Distilling {
  enum class Reason {
    kNone,
    kAutomatic,  // Speedreader mode.
    kManual,     // Reader mode toggle is clicked or settings have been changed.
  } reason = Reason::kNone;

  explicit Distilling(Reason reason);
};

struct Distilled : Distilling {
  using Reason = Distilling::Reason;
  DistillationResult result = DistillationResult::kNone;

  explicit Distilled(DistillationResult result);
  Distilled(Reason reason, DistillationResult result);
  Distilled(const Distilling& state, DistillationResult result);
};

struct DistillReverting : ViewOriginal {
  using Reason = ViewOriginal::Reason;

  using ViewOriginal::ViewOriginal;
  DistillReverting(const Distilling& state, Reason reason);
  DistillReverting(const Distilled& state, Reason reason);
};

using State = std::variant<DistillStates::None,
                           DistillStates::ViewOriginal,
                           DistillStates::Distilling,
                           DistillStates::Distilled,
                           DistillStates::DistillReverting>;

bool IsViewOriginal(const State& state);
bool IsDistilling(const State& state);
bool IsDistilled(const State& state);
bool IsDistillReverting(const State& state);

bool IsNotDistillable(const State& state);
bool IsDistillable(const State& state);
bool IsDistilledAutomatically(const State& state);

// Performs the transition from |state| to |desired|, returns true if transition
// requires page reload.
bool Transit(State& state, const State& desired);

}  // namespace DistillStates

using DistillState = DistillStates::State;

using DistillationResultCallback =
    base::OnceCallback<void(DistillationResult result,
                            std::string original_data,
                            std::string transformed)>;
void DistillPage(const GURL& url,
                 std::string body,
                 SpeedreaderService* speedreader_service,
                 SpeedreaderRewriterService* rewriter_service,
                 DistillationResultCallback callback);

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_
