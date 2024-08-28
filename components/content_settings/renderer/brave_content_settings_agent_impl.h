/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "brave/components/brave_shields/core/common/brave_shields.mojom.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom-forward.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/renderer/content_settings_agent_impl.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "third_party/blink/public/web/web_frame.h"
#include "url/gurl.h"

namespace blink {
class WebLocalFrame;
}

namespace content_settings {

// Handles blocking content per content settings for each RenderFrame.
class BraveContentSettingsAgentImpl
    : public ContentSettingsAgentImpl,
      public brave_shields::mojom::BraveShields {
 public:
  BraveContentSettingsAgentImpl(content::RenderFrame* render_frame,
                                std::unique_ptr<Delegate> delegate);
  BraveContentSettingsAgentImpl(const BraveContentSettingsAgentImpl&) = delete;
  BraveContentSettingsAgentImpl& operator=(
      const BraveContentSettingsAgentImpl&) = delete;
  ~BraveContentSettingsAgentImpl() override;

  bool IsCosmeticFilteringEnabled(const GURL& url) override;

  bool IsFirstPartyCosmeticFilteringEnabled(const GURL& url) override;

  // RenderFrameObserver:
  void DidCommitProvisionalLoad(ui::PageTransition transition) override;

  const brave_shields::mojom::ShieldsSettingsPtr& shields_settings() const {
    return shields_settings_;
  }

 protected:
  bool AllowScript(bool enabled_per_settings) override;
  bool AllowScriptFromSource(bool enabled_per_settings,
                             const blink::WebURL& script_url) override;
  void DidNotAllowScript() override;

  blink::WebSecurityOrigin GetEphemeralStorageOriginSync() override;
  bool AllowStorageAccessSync(StorageType storage_type) override;

  void BraveSpecificDidBlockJavaScript(const std::u16string& details);
  void BraveSpecificDidAllowJavaScriptOnce(const GURL& details);
  bool AllowAutoplay(bool play_requested) override;

  brave_shields::mojom::ShieldsSettingsPtr GetBraveShieldsSettings(
      ContentSettingsType webcompat_settings_type) override;

  bool IsReduceLanguageEnabled() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveContentSettingsAgentImplAutoplayBrowserTest,
                           AutoplayBlockedByDefault);
  FRIEND_TEST_ALL_PREFIXES(BraveContentSettingsAgentImplAutoplayBrowserTest,
                           AutoplayAllowedByDefault);

  bool IsBraveShieldsDown(const blink::WebFrame* frame,
                          const GURL& secondary_url);

  bool IsScriptTemporarilyAllowed(const GURL& script_url);

  // brave_shields::mojom::BraveShields.
  void SetShieldsSettings(
      brave_shields::mojom::ShieldsSettingsPtr settings) override;

  void BindBraveShieldsReceiver(
      mojo::PendingAssociatedReceiver<brave_shields::mojom::BraveShields>
          pending_receiver);

  // Returns the associated remote used to send messages to the browser process,
  // lazily initializing it the first time it's used.
  mojo::AssociatedRemote<brave_shields::mojom::BraveShieldsHost>&
  GetOrCreateBraveShieldsRemote();

  // cache blocked script url which will later be used in `DidNotAllowScript()`
  GURL blocked_script_url_;

  brave_shields::mojom::ShieldsSettingsPtr shields_settings_;

  base::flat_map<url::Origin, blink::WebSecurityOrigin>
      cached_ephemeral_storage_origins_;

  mojo::AssociatedRemote<brave_shields::mojom::BraveShieldsHost>
      brave_shields_remote_;

  mojo::AssociatedReceiverSet<brave_shields::mojom::BraveShields>
      brave_shields_receivers_;
};

}  // namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_RENDERER_BRAVE_CONTENT_SETTINGS_AGENT_IMPL_H_
