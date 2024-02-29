// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_WAIT_FOR_CALLBACK_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_WAIT_FOR_CALLBACK_H_

#include <optional>
#include <tuple>
#include <utility>

#include "base/functional/callback_forward.h"
#include "base/run_loop.h"
#include "base/test/bind.h"

// A helper function for Brave News tests which waits for a callback to be
// invoked and returns the result.
template <typename... CallbackArgs>
std::tuple<std::decay_t<CallbackArgs>...> WaitForCallback(
    base::OnceCallback<void(base::OnceCallback<void(CallbackArgs...)>)> run) {
  std::optional<std::tuple<std::decay_t<CallbackArgs>...>> result =
      std::nullopt;

  base::RunLoop loop;

  std::move(run).Run(
      base::BindLambdaForTesting([&loop, &result](CallbackArgs... args) {
        result.emplace(std::forward<CallbackArgs>(args)...);
        loop.Quit();
      }));

  loop.Run();
  return std::move(result).value();
}

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_WAIT_FOR_CALLBACK_H_
