/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "brave/third_party/blink/renderer/brave_font_whitelist.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/test/render_view_test.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

using brave_shields::features::kBraveReduceLanguage;

namespace {

const char kFontLocalSourceHTML[] =
    "<html><head><style>@font-face{font-family:Helvetica "
    "Shadow;src:local('Helvetica')}</style></head><body><p><span id='test1' "
    "style=\"font-family: 'Helvetica'\">mmMwWLliI0fiflO&1</span></p><p><span "
    "id='test2' "
    "style=\"font-family: 'Helvetica "
    "Shadow'\">mmMwWLliI0fiflO&1</span></p></body></html>";

}  // namespace

namespace content_settings {

namespace {

class MockContentSettingsAgentImpl : public BraveContentSettingsAgentImpl {
 public:
  explicit MockContentSettingsAgentImpl(content::RenderFrame* render_frame)
      : BraveContentSettingsAgentImpl(
            render_frame,
            std::make_unique<ContentSettingsAgentImpl::Delegate>()) {}

  bool IsReduceLanguageEnabled() override { return true; }
};

}  // namespace

class BraveFontWhitelistRenderViewTest : public content::RenderViewTest {
 public:
  BraveFontWhitelistRenderViewTest() {
    feature_list_.InitAndEnableFeature(kBraveReduceLanguage);
  }
  ~BraveFontWhitelistRenderViewTest() override = default;

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Test that pages can not use a src:local CSS declaration to bypass the font
// whitelist. This test requires a specific font to be installed locally, so we
// only run the test on Mac.
#if BUILDFLAG(IS_MAC)
#define MAYBE_FontLocalSource FontLocalSource
#else
#define MAYBE_FontLocalSource DISABLED_FontLocalSource
#endif
TEST_F(BraveFontWhitelistRenderViewTest, MAYBE_FontLocalSource) {
  // Simulate an empty font whitelist. This creates a situation where we know
  // there is a font installed locally (Helvetica, preinstalled on every Mac)
  // that is not on the font whitelist.
  brave::SetSimulateEmptyFontWhitelistForTesting(true);

  // Use mock content settings agent that unconditionally enables font
  // whitelisting.
  MockContentSettingsAgentImpl agent(GetMainRenderFrame());

  // Load test HTML page with two specially constructed <span> elements. The
  // first tries to use Helvetica directly, which will be blocked (even though
  // the font exists locally) because it's not on the whitelist. The second
  // tries to use Helvetica via a shadow src:local font declaration, which
  // previously succeeded (thereby bypassing the font whitelist) because
  // src:local CSS font processing was not hooked into the font whitelist logic
  // in brave::AllowFontFamily.
  LoadHTMLWithUrlOverride(kFontLocalSourceHTML,
                          GURL("http://b.test/").spec().c_str());
  blink::WebDocument document = GetMainFrame()->GetDocument();
  blink::WebElement p1 =
      document.GetElementById(blink::WebString::FromUTF8("test1"));
  ASSERT_FALSE(p1.IsNull());
  blink::WebElement p2 =
      document.GetElementById(blink::WebString::FromUTF8("test2"));
  ASSERT_FALSE(p2.IsNull());

  // If the width of both spans is the same, that means they were both blocked
  // from using the specified font (Helvetica), which is what we want.
  EXPECT_EQ(p1.BoundsInWidget().width(), p2.BoundsInWidget().width());

  brave::SetSimulateEmptyFontWhitelistForTesting(false);
}

}  // namespace content_settings
