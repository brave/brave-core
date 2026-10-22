// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_CONTENT_SHIELDS_SETTINGS_AGENT_H_
#define BRAVE_IOS_WEB_CONTENT_SHIELDS_SETTINGS_AGENT_H_

#include "brave/components/brave_shields/core/common/brave_shields.mojom.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

namespace brave_shields {

// Hands the Shields settings pushed by ShieldsSettingsSender to Brave's Blink
// code (farbling, language reduction). A lighter stand-in for desktop's
// BraveContentSettingsAgentImpl, which needs Chrome's content settings stack.
class ShieldsSettingsAgent : public content::RenderFrameObserver,
                             public blink::WebContentSettingsClient,
                             public mojom::BraveShields {
 public:
  explicit ShieldsSettingsAgent(content::RenderFrame* render_frame);
  ShieldsSettingsAgent(const ShieldsSettingsAgent&) = delete;
  ShieldsSettingsAgent& operator=(const ShieldsSettingsAgent&) = delete;

  // blink::WebContentSettingsClient:
  mojom::ShieldsSettingsPtr GetBraveShieldsSettings(
      ContentSettingsType webcompat_settings_type) override;
  bool IsReduceLanguageEnabled() override;
  bool HasContentSettingsRules() const override;

  // mojom::BraveShields:
  void SetShieldsSettings(mojom::ShieldsSettingsPtr settings) override;

 private:
  ~ShieldsSettingsAgent() override;

  // content::RenderFrameObserver:
  void OnDestruct() override;

  void BindReceiver(
      mojo::PendingAssociatedReceiver<mojom::BraveShields> receiver);

  mojom::ShieldsSettingsPtr settings_;
  mojo::AssociatedReceiverSet<mojom::BraveShields> receivers_;
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_WEB_CONTENT_SHIELDS_SETTINGS_AGENT_H_
