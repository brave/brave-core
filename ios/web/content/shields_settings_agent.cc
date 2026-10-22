// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/content/shields_settings_agent.h"

#include <utility>

#include "base/functional/bind.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace brave_shields {

ShieldsSettingsAgent::ShieldsSettingsAgent(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetWebFrame()->SetContentSettingsClient(this);
  render_frame->GetAssociatedInterfaceRegistry()
      ->AddInterface<mojom::BraveShields>(base::BindRepeating(
          &ShieldsSettingsAgent::BindReceiver, base::Unretained(this)));
}

ShieldsSettingsAgent::~ShieldsSettingsAgent() = default;

mojom::ShieldsSettingsPtr ShieldsSettingsAgent::GetBraveShieldsSettings(
    ContentSettingsType webcompat_settings_type) {
  return settings_ ? settings_.Clone() : mojom::ShieldsSettings::New();
}

bool ShieldsSettingsAgent::IsReduceLanguageEnabled() {
  return settings_ && settings_->reduce_language;
}

// Brave's Blink code ignores clients without rules.
bool ShieldsSettingsAgent::HasContentSettingsRules() const {
  return true;
}

void ShieldsSettingsAgent::SetShieldsSettings(
    mojom::ShieldsSettingsPtr settings) {
  settings_ = std::move(settings);
}

void ShieldsSettingsAgent::OnDestruct() {
  delete this;
}

void ShieldsSettingsAgent::BindReceiver(
    mojo::PendingAssociatedReceiver<mojom::BraveShields> receiver) {
  receivers_.Add(this, std::move(receiver));
}

}  // namespace brave_shields
