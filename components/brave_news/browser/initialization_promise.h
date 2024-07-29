// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_INITIALIZATION_PROMISE_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_INITIALIZATION_PROMISE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-params-data.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"

namespace brave_news {

class BraveNewsPrefManager;

class InitializationPromise {
 public:
  using GetLocale = base::RepeatingCallback<void(
      mojom::BraveNewsController::GetLocaleCallback)>;
  enum class State { kNone, kInitializing, kInitialized, kFailed };

  InitializationPromise(size_t max_retries,
                        BraveNewsPrefManager& pref_manager,
                        GetLocale get_locale);
  ~InitializationPromise();

  void OnceInitialized(base::OnceClosure on_initialized);

  State state() const { return state_; }
  bool failed() const {
    return state_ == InitializationPromise::State::kFailed;
  }
  bool complete() const {
    return on_initializing_prefs_complete_.is_signaled();
  }

  void set_no_retry_delay_for_testing(bool no_retry_delay) {
    no_retry_delay_for_testing_ = no_retry_delay;
  }

  size_t attempts_for_testing() const { return attempts_; }

 private:
  void Initialize();
  void OnGotLocale(const std::string& locale);

  void Notify();

  raw_ref<BraveNewsPrefManager> pref_manager_;
  GetLocale get_locale_;

  base::OneShotEvent on_initializing_prefs_complete_;

  bool no_retry_delay_for_testing_ = false;

  const size_t max_retries_ = 0;
  size_t attempts_ = 0;

  State state_ = InitializationPromise::State::kNone;

  base::WeakPtrFactory<InitializationPromise> weak_ptr_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_INITIALIZATION_PROMISE_H_
