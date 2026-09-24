// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/functional/callback_helpers.h"
#include "brave/components/script_injector/renderer/script_injector_render_frame_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace script_injector {

TEST(ScriptInjectorUnitTest, CheckIfWantResultHasCallback) {
  ScriptInjectorRenderFrameObserver::RequestAsyncExecuteScriptCallback
      callback = base::DoNothing();
  EXPECT_EQ(
      ScriptInjectorRenderFrameObserver::CheckIfWantResult(std::move(callback)),
      blink::mojom::WantResultOption::kWantResult);
}

TEST(ScriptInjectorUnitTest, CheckIfWantResultNullCallback) {
  EXPECT_EQ(ScriptInjectorRenderFrameObserver::CheckIfWantResult(
                base::NullCallback()),
            blink::mojom::WantResultOption::kNoResult);
}

}  // namespace script_injector
