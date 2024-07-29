// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/initialization_promise.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/location.h"
#include "base/memory/raw_ref.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"

namespace brave_news {

constexpr int kBackoffs[] = {1, 5, 10};
constexpr size_t kNumBackoffs = std::size(kBackoffs);

InitializationPromise::InitializationPromise(size_t max_retries,
                                             BraveNewsPrefManager& pref_manager,
                                             GetLocale get_locale)
    : pref_manager_(pref_manager),
      get_locale_(get_locale),
      max_retries_(max_retries) {}

InitializationPromise::~InitializationPromise() = default;

void InitializationPromise::OnceInitialized(base::OnceClosure on_initialized) {
  // This should only be called when Brave News is enabled.
  CHECK(pref_manager_->IsEnabled());

  if (state_ == State::kInitialized || state_ == State::kFailed) {
    std::move(on_initialized).Run();
    return;
  }

  on_initializing_prefs_complete_.Post(FROM_HERE, std::move(on_initialized));

  // Only start initializing once.
  if (state_ == State::kNone) {
    Initialize();
  }
}

void InitializationPromise::Initialize() {
  state_ = State::kInitializing;

  auto subscriptions = pref_manager_->GetSubscriptions();

  // If things are already initialized, we're done!
  if (!subscriptions.channels().empty() ||
      !subscriptions.enabled_publishers().empty() ||
      !subscriptions.disabled_publishers().empty() ||
      !subscriptions.direct_feeds().empty()) {
    state_ = State::kInitialized;
    on_initializing_prefs_complete_.Signal();
    return;
  }

  get_locale_.Run(base::BindOnce(&InitializationPromise::OnGotLocale,
                                 weak_ptr_factory_.GetWeakPtr()));
}

void InitializationPromise::OnGotLocale(const std::string& locale) {
  CHECK_EQ(State::kInitializing, state_);

  // Keep track of which attempt this is.
  attempts_++;

  if (!locale.empty()) {
    pref_manager_->SetChannelSubscribed(locale, kTopSourcesChannel, true);
    state_ = State::kInitialized;
    on_initializing_prefs_complete_.Signal();
    return;
  }

  // We signal even if nothing managed to initialized because otherwise we'll
  // get stuck.
  if (attempts_ >= max_retries_) {
    state_ = State::kFailed;
    on_initializing_prefs_complete_.Signal();
    return;
  }

  // Determine how long we should wait based on the number of times we've
  // retried.
  int retry_delay = no_retry_delay_for_testing_
                        ? 0
                        : kBackoffs[std::min(attempts_, kNumBackoffs - 1)];
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&InitializationPromise::Initialize,
                     weak_ptr_factory_.GetWeakPtr()),
      base::Seconds(retry_delay));
}

}  // namespace brave_news
