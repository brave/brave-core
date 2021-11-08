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
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace content_settings {
namespace {

class MockContentSettingsManagerImpl : public mojom::ContentSettingsManager {
 public:
  struct Log {
    int on_content_blocked_count = 0;
    ContentSettingsType on_content_blocked_type = ContentSettingsType::DEFAULT;
  };

  explicit MockContentSettingsManagerImpl(Log* log) : log_(log) {}
  ~MockContentSettingsManagerImpl() override = default;

  // mojom::ContentSettingsManager methods:
  void Clone(
      mojo::PendingReceiver<mojom::ContentSettingsManager> receiver) override {
    ADD_FAILURE() << "Not reached";
  }

  void AllowStorageAccess(int32_t render_frame_id,
                          StorageType storage_type,
                          const url::Origin& origin,
                          const ::net::SiteForCookies& site_for_cookies,
                          const url::Origin& top_frame_origin,
                          base::OnceCallback<void(bool)> callback) override {}

  void AllowEphemeralStorageAccess(
      int32_t render_frame_id,
      const ::url::Origin& origin,
      const ::net::SiteForCookies& site_for_cookies,
      const ::url::Origin& top_frame_origin,
      AllowEphemeralStorageAccessCallback callback) override {}

  void OnContentBlocked(int32_t render_frame_id,
                        ContentSettingsType type) override {
    ++log_->on_content_blocked_count;
    log_->on_content_blocked_type = type;
  }

 private:
  Log* log_;
};

class MockContentSettingsAgentImpl : public BraveContentSettingsAgentImpl {
 public:
  explicit MockContentSettingsAgentImpl(content::RenderFrame* render_frame);
  MockContentSettingsAgentImpl(const MockContentSettingsAgentImpl&) = delete;
  MockContentSettingsAgentImpl& operator=(const MockContentSettingsAgentImpl&) =
      delete;
  ~MockContentSettingsAgentImpl() override {}

  // ContentSettingAgentImpl methods:
  void BindContentSettingsManager(
      mojo::Remote<mojom::ContentSettingsManager>* manager) override;

  int on_content_blocked_count() const { return log_.on_content_blocked_count; }
  ContentSettingsType on_content_blocked_type() const {
    return log_.on_content_blocked_type;
  }

 private:
  MockContentSettingsManagerImpl::Log log_;
};

MockContentSettingsAgentImpl::MockContentSettingsAgentImpl(
    content::RenderFrame* render_frame)
    : BraveContentSettingsAgentImpl(
          render_frame,
          false,
          std::make_unique<ContentSettingsAgentImpl::Delegate>()) {}

void MockContentSettingsAgentImpl::BindContentSettingsManager(
    mojo::Remote<mojom::ContentSettingsManager>* manager) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<MockContentSettingsManagerImpl>(&log_),
      manager->BindNewPipeAndPassReceiver());
}
}  // namespace

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
    GetMainRenderFrame()->GetAssociatedInterfaceRegistry()->RemoveInterface(
        mojom::ContentSettingsAgent::Name_);
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

  MockContentSettingsAgentImpl agent(GetMainRenderFrame());
  agent.SetContentSettingRules(&content_setting_rules);
  EXPECT_FALSE(agent.AllowAutoplay(true));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, agent.on_content_blocked_count());
  EXPECT_EQ(ContentSettingsType::AUTOPLAY, agent.on_content_blocked_type());

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

  MockContentSettingsAgentImpl agent(GetMainRenderFrame());
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
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, agent.on_content_blocked_count());
  EXPECT_EQ(ContentSettingsType::AUTOPLAY, agent.on_content_blocked_type());
}

}  // namespace content_settings
