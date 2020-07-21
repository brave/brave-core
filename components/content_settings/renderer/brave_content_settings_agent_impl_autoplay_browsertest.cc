/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/renderer/content_settings_agent_impl.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "content/public/test/render_view_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace content_settings {

class BraveContentSettingsAgentImplAutoplayBrowserTest
    : public content::RenderViewTest {
 protected:
  void SetUp() override {
    RenderViewTest::SetUp();

    // Set up a fake url loader factory to ensure that script loader can create
    // a WebURLLoader.
    CreateFakeWebURLLoaderFactory();

    // Unbind the ContentSettingsAgent interface that would be registered by
    // the ContentSettingsAgentImpl created when the render frame is created.
    view_->GetMainRenderFrame()
        ->GetAssociatedInterfaceRegistry()
        ->RemoveInterface(mojom::ContentSettingsAgent::Name_);
  }
};

TEST_F(BraveContentSettingsAgentImplAutoplayBrowserTest,
       AutoplayBlockedByDefault) {
  LoadHTMLWithUrlOverride("<html>Autoplay</html>", "https://example.com/");

  // Set the default autoplay blocking setting.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& autoplay_setting_rules =
      content_setting_rules.autoplay_rules;
  autoplay_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
      std::string(), false));

  BraveContentSettingsAgentImpl agent(
      view_->GetMainRenderFrame(), false,
      std::make_unique<ContentSettingsAgentImpl::Delegate>());
  agent.SetContentSettingRules(&content_setting_rules);
  EXPECT_FALSE(agent.AllowAutoplay(true));

  // Create an exception which allows the autoplay.
  autoplay_setting_rules.insert(
      autoplay_setting_rules.begin(),
      ContentSettingPatternSource(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::FromString("https://example.com"),
          base::Value::FromUniquePtrValue(
              content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
          std::string(), false));
  EXPECT_TRUE(agent.AllowAutoplay(true));
}

TEST_F(BraveContentSettingsAgentImplAutoplayBrowserTest,
       AutoplayAllowedByDefault) {
  LoadHTMLWithUrlOverride("<html>Autoplay</html>", "https://example.com/");

  // Set the default autoplay blocking setting.
  RendererContentSettingRules content_setting_rules;
  ContentSettingsForOneType& autoplay_setting_rules =
      content_setting_rules.autoplay_rules;
  autoplay_setting_rules.push_back(ContentSettingPatternSource(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      base::Value::FromUniquePtrValue(
          content_settings::ContentSettingToValue(CONTENT_SETTING_ALLOW)),
      std::string(), false));

  BraveContentSettingsAgentImpl agent(
      view_->GetMainRenderFrame(), false,
      std::make_unique<ContentSettingsAgentImpl::Delegate>());
  agent.SetContentSettingRules(&content_setting_rules);
  EXPECT_TRUE(agent.AllowAutoplay(true));

  // Create an exception which allows the autoplay.
  autoplay_setting_rules.insert(
      autoplay_setting_rules.begin(),
      ContentSettingPatternSource(
          ContentSettingsPattern::Wildcard(),
          ContentSettingsPattern::FromString("https://example.com"),
          base::Value::FromUniquePtrValue(
              content_settings::ContentSettingToValue(CONTENT_SETTING_BLOCK)),
          std::string(), false));
  EXPECT_FALSE(agent.AllowAutoplay(true));
}

}  // namespace content_settings
