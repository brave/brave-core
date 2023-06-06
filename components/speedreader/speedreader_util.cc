/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_util.h"

#include <memory>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/speedreader/rust/ffi/speedreader.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "url/gurl.h"

namespace speedreader {

namespace DistillStates {

bool IsTransition(const State& state) {
  if (absl::holds_alternative<None>(state)) {
    return false;
  }
  if (const auto* s = absl::get_if<ViewOriginal>(&state)) {
    return s->reason == ViewOriginal::Reason::kUserAction;
  }
  if (IsPending(state)) {
    return true;
  } else if (IsDistilled(state)) {
    return false;
  }
  NOTREACHED();
  return false;
}

bool IsPending(const State& state) {
  return absl::holds_alternative<Pending>(state);
}

bool IsDistilled(const State& state) {
  return absl::holds_alternative<Distilled>(state);
}

bool IsDistillable(const State& state) {
  return IsDistilled(state) ||
         (IsViewOriginal(state) &&
          absl::get<DistillStates::ViewOriginal>(state).reason !=
              DistillStates::ViewOriginal::Reason::kError);
}

bool IsViewOriginal(const State& state) {
  return absl::holds_alternative<ViewOriginal>(state);
}

bool IsNotDistillable(const State& state) {
  return IsViewOriginal(state) &&
         absl::get<DistillStates::ViewOriginal>(state).reason ==
             DistillStates::ViewOriginal::Reason::kError;
}

}  // namespace DistillStates

bool TransitToDistilledDirection(
    DistillState& state,
    DistillStates::Pending::Reason reason,
    DistillationResult result = DistillationResult::kNone) {
  if (absl::holds_alternative<DistillStates::Pending>(state)) {
    if (result == DistillationResult::kSuccess) {
      state = DistillStates::Distilled(result);
      return false;
    } else {
      // Distillation failed.
      state = DistillStates::ViewOriginal(
          DistillStates::ViewOriginal::Reason::kError);
    }
    return false;
  }
  if (absl::holds_alternative<DistillStates::ViewOriginal>(state)) {
    state = DistillStates::Pending(reason);
    return true;
  }
  if (absl::holds_alternative<DistillStates::Distilled>(state)) {
    // Already distilled.
    return false;
  }

  NOTREACHED();
  return false;
}

bool TransitToOriginalDirection(DistillState& state, DistillState&& desired) {
  if (absl::holds_alternative<DistillStates::ViewOriginal>(state)) {
    // Already shows original.
    state = std::move(desired);
    return false;
  }
  if (absl::holds_alternative<DistillStates::Pending>(state) ||
      absl::holds_alternative<DistillStates::Distilled>(state)) {
    state = std::move(desired);
    return true;
  }

  NOTREACHED();
  return false;
}

bool Transit(DistillState& state, DistillState&& desired) {
  if (absl::holds_alternative<DistillStates::None>(desired)) {
    // Nothing to do.
  } else if (absl::holds_alternative<DistillStates::ViewOriginal>(desired)) {
    return TransitToOriginalDirection(state, std::move(desired));
  } else if (absl::holds_alternative<DistillStates::Pending>(desired)) {
    return TransitToDistilledDirection(
        state, absl::get<DistillStates::Pending>(desired).reason);
  } else {
    CHECK(absl::holds_alternative<DistillStates::Distilled>(desired));
    return TransitToDistilledDirection(
        state, DistillStates::Pending::Reason::kNone,
        absl::get<DistillStates::Distilled>(desired).result);
  }
  return false;
}

void PerformStateTransition(DistillState& state) {
  if (absl::holds_alternative<DistillStates::None>(state)) {
    NOTREACHED() << "'None' is not transition state.";
    return;
  }
  if (auto* s = absl::get_if<DistillStates::ViewOriginal>(&state)) {
    s->reason = DistillStates::ViewOriginal::Reason::kNone;
  }
}

void DistillPage(const GURL& url,
                 std::string body,
                 SpeedreaderService* speedreader_service,
                 SpeedreaderRewriterService* rewriter_service,
                 DistillationResultCallback callback) {
  struct Result {
    DistillationResult result;
    std::string body;
    std::string transformed;
  };

  auto distill = [](const GURL& url, std::string data,
                    std::unique_ptr<Rewriter> rewriter) -> Result {
    SCOPED_UMA_HISTOGRAM_TIMER("Brave.Speedreader.Distill");
    int written = rewriter->Write(data.c_str(), data.length());
    // Error occurred
    if (written != 0) {
      return {DistillationResult::kFail, std::move(data), std::string()};
    }

    rewriter->End();
    const std::string& transformed = rewriter->GetOutput();

    // If the distillation failed, the rewriter returns an empty string. Also,
    // if the output is too small, we assume that the content of the distilled
    // page does not contain enough text to read.
    if (transformed.length() < 1024) {
      return {DistillationResult::kFail, std::move(data), std::string()};
    }
    return {DistillationResult::kSuccess, std::move(data), transformed};
  };

  auto return_result = [](DistillationResultCallback callback, Result r) {
    std::move(callback).Run(r.result, r.body, r.transformed);
  };

  auto rewriter = rewriter_service->MakeRewriter(
      url, speedreader_service->GetThemeName(),
      speedreader_service->GetFontFamilyName(),
      speedreader_service->GetFontSizeName(), "");

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(distill, url, std::move(body), std::move(rewriter)),
      base::BindOnce(return_result, std::move(callback)));
}

}  // namespace speedreader
