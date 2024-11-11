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

ViewOriginal::ViewOriginal() = default;

ViewOriginal::ViewOriginal(ViewOriginal::Reason reason, bool was_auto_distilled)
    : reason(reason), was_auto_distilled(was_auto_distilled) {}

ViewOriginal::ViewOriginal(const DistillReverting& state)
    : ViewOriginal(state.reason, state.was_auto_distilled) {}

Distilling::Distilling(Distilling::Reason reason) : reason(reason) {}

Distilled::Distilled(DistillationResult result)
    : Distilling(Distilling::Reason::kNone), result(result) {}

Distilled::Distilled(Distilled::Reason reason, DistillationResult result)
    : Distilling(reason), result(result) {}

Distilled::Distilled(const Distilling& state, DistillationResult result)
    : Distilled(state.reason, result) {}

DistillReverting::DistillReverting(const Distilling& state, Reason reason)
    : ViewOriginal(reason, state.reason == Distilling::Reason::kAutomatic) {}

DistillReverting::DistillReverting(const Distilled& state,
                                   DistillReverting::Reason reason)
    : ViewOriginal(reason, state.reason == Distilled::Reason::kAutomatic) {}

bool IsViewOriginal(const State& state) {
  return absl::holds_alternative<ViewOriginal>(state);
}

bool IsDistilling(const State& state) {
  return absl::holds_alternative<Distilling>(state);
}

bool IsDistilled(const State& state) {
  return absl::holds_alternative<Distilled>(state);
}

bool IsDistillReverting(const State& state) {
  return absl::holds_alternative<DistillReverting>(state);
}

bool IsNotDistillable(const State& state) {
  return IsViewOriginal(state) &&
         absl::get<DistillStates::ViewOriginal>(state).reason ==
             DistillStates::ViewOriginal::Reason::kNotDistillable;
}

bool IsDistillable(const State& state) {
  return IsViewOriginal(state) &&
         absl::get<DistillStates::ViewOriginal>(state).reason !=
             DistillStates::ViewOriginal::Reason::kNotDistillable;
}

bool IsDistilledAutomatically(const State& state) {
  if (const auto* d = absl::get_if<Distilled>(&state)) {
    return d->reason == Distilled::Reason::kAutomatic;
  }
  return false;
}

bool Transit(State& state, const State& desired) {
  if (absl::holds_alternative<None>(state)) {
    state = desired;
    return false;
  }

  if (IsViewOriginal(state)) {
    if (IsDistillReverting(desired)) {
      state = ViewOriginal(absl::get<DistillReverting>(desired));
      return false;
    }
    if (IsDistilling(desired)) {
      state = desired;
      return true;
    }
    if (IsDistilled(desired)) {
      state = Distilling(absl::get<Distilled>(desired));
      return true;
    }
    DCHECK(IsViewOriginal(desired));
    // Already view original
    return false;
  }

  if (IsDistillReverting(state)) {
    if (IsViewOriginal(desired)) {
      state = ViewOriginal(absl::get<DistillReverting>(state));
      return false;
    }
  }

  if (IsDistilling(state)) {
    if (IsDistilled(desired)) {
      const auto& d = absl::get<Distilled>(desired);
      if (d.result == DistillationResult::kFail) {
        state = ViewOriginal(ViewOriginal::Reason::kError, false);
        return false;
      }
      state = Distilled(absl::get<Distilling>(state), d.result);
      return false;
    }
    if (IsDistillReverting(desired) || IsDistilling(desired) ||
        IsViewOriginal(desired)) {
      state = desired;
      return false;
    }
  }

  if (IsDistilled(state)) {
    if (IsDistillReverting(desired)) {
      state = desired;
      return true;
    }
    if (IsViewOriginal(desired)) {
      state = DistillReverting(
          absl::get<ViewOriginal>(desired).reason,
          absl::get<Distilled>(state).reason == Distilled::Reason::kAutomatic);
      return true;
    }
    if (IsDistilled(desired) || IsDistilling(desired)) {
      // Already distilled.
      return false;
    }
  }

  NOTREACHED();
}

}  // namespace DistillStates

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

  auto rewriter =
      rewriter_service->MakeRewriter(url, speedreader_service->GetThemeName(),
                                     speedreader_service->GetFontFamilyName(),
                                     speedreader_service->GetFontSizeName(),
                                     speedreader_service->GetColumnWidth());
  if (!rewriter) {
    std::move(callback).Run(DistillationResult::kFail, std::move(body),
                            std::string());
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::USER_BLOCKING, base::MayBlock()},
      base::BindOnce(distill, url, std::move(body), std::move(rewriter)),
      base::BindOnce(return_result, std::move(callback)));
}

}  // namespace speedreader
