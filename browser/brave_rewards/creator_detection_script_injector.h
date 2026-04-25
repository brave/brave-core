// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_CREATOR_DETECTION_SCRIPT_INJECTOR_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_CREATOR_DETECTION_SCRIPT_INJECTOR_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "content/public/browser/global_routing_id.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "url/gurl.h"

namespace content {
class RenderFrameHost;
}

namespace brave_rewards {

// Responsible for detecting Brave creator information associated with media
// platform pages, using JS scripts that are injected into an isolated world.
class CreatorDetectionScriptInjector {
 public:
  CreatorDetectionScriptInjector();
  ~CreatorDetectionScriptInjector();

  CreatorDetectionScriptInjector(const CreatorDetectionScriptInjector&) =
      delete;
  CreatorDetectionScriptInjector& operator=(
      const CreatorDetectionScriptInjector&) = delete;

  // Injects creator detection scripts (if appropriate) into an isolated world
  // associated with the specified render frame host. The scripts are expected
  // to set up a JS function that will later be called by `DetectCreator`.
  void MaybeInjectScript(content::RenderFrameHost* rfh);

  struct Result {
    Result();
    ~Result();
    Result(const Result&);
    Result& operator=(const Result&);

    std::string id;
    std::string name;
    std::string url;
    std::string image_url;
  };

  using DetectCreatorCallback = base::OnceCallback<void(std::optional<Result>)>;

  // Runs the creator detection routine initialized by `MaybeInjectScript` and
  // asynchrounously returns the detection result. Returns `nullopt` if the
  // detection routine was not invoked (e.g. because Rewards is not enabled or
  // because there is no script for this page). Returns a `Result` with empty
  // fields if there is no creator associated with the current page. Note that
  // any of the `Result` fields may be empty if the detection script was unable
  // to gather that information from the page.
  void DetectCreator(content::RenderFrameHost* rfh,
                     DetectCreatorCallback callback);

 private:
  using ExecuteScriptCallback = base::OnceCallback<void(base::Value)>;

  void ExecuteScript(std::string_view script, ExecuteScriptCallback callback);

  void OnDetectionCancelled(DetectCreatorCallback callback);

  void OnCreatorDetected(DetectCreatorCallback callback,
                         uint64_t request_id,
                         base::Value value);

  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> injector_;
  content::GlobalRenderFrameHostToken injector_host_token_;
  GURL last_detection_url_;
  uint64_t current_request_id_ = 0;
  base::WeakPtrFactory<CreatorDetectionScriptInjector> weak_factory_{this};
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_CREATOR_DETECTION_SCRIPT_INJECTOR_H_
