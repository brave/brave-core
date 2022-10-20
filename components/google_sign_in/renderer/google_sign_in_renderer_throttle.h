/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GOOGLE_SIGN_IN_RENDERER_GOOGLE_SIGN_IN_RENDERER_THROTTLE_H_
#define BRAVE_COMPONENTS_GOOGLE_SIGN_IN_RENDERER_GOOGLE_SIGN_IN_RENDERER_THROTTLE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"

namespace blink {
class WebURLRequest;
}  // namespace blink

namespace google_sign_in {

class GoogleSignInRendererThrottle : public blink::URLLoaderThrottle {
 public:
  GoogleSignInRendererThrottle();
  ~GoogleSignInRendererThrottle() override;

  static std::unique_ptr<blink::URLLoaderThrottle> MaybeCreateThrottleFor(
      int render_frame_id,
      const blink::WebURLRequest& request);

  GoogleSignInRendererThrottle(const GoogleSignInRendererThrottle&) = delete;
  GoogleSignInRendererThrottle& operator=(const GoogleSignInRendererThrottle&) =
      delete;

  // Implements blink::URLLoaderThrottle:
  void DetachFromCurrentSequence() override;
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override;

 private:
  base::WeakPtrFactory<GoogleSignInRendererThrottle> weak_factory_{this};
};

}  // namespace google_sign_in

#endif  // BRAVE_COMPONENTS_GOOGLE_SIGN_IN_RENDERER_GOOGLE_SIGN_IN_RENDERER_THROTTLE_H_
