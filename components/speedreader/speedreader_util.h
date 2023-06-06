/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_UTIL_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "third_party/abseil-cpp/absl/types/variant.h"
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

using None = absl::monostate;

struct ViewOriginal {
  enum class Reason {
    kNone,        // Original page shown because no action was performed.
    kUserAction,  // Reader mode toggle clicked or settings changed.
    kError,       // Original page shown because distillation was failed.
  } reason = Reason::kNone;

  GURL url;
};

struct Pending {
  enum class Reason {
    kNone,
    kAutomatic,  // Speedreader mode.
    kManual,     // Reader mode toggle clicked or settings changed.
  } reason = Reason::kNone;
};

struct Distilled {
  DistillationResult result = DistillationResult::kNone;
};

using State = absl::variant<DistillStates::None,
                            DistillStates::ViewOriginal,
                            DistillStates::Pending,
                            DistillStates::Distilled>;

bool IsTransition(const State& state);
bool IsPending(const State& state);
bool IsDistilled(const State& state);
bool IsDistillable(const State& state);
bool IsViewOriginal(const State& state);
bool IsNotDistillable(const State& state);

}  // namespace DistillStates

using DistillState = DistillStates::State;

void PerformStateTransition(DistillState& state);
bool Transit(DistillState& state, DistillState&& desired);

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
