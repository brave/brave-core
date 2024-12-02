/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_browsertest_util.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "third_party/blink/public/mojom/context_menu/context_menu.mojom.h"

class BraveContextMenuBrowserTest : public InProcessBrowserTest {
 protected:
  BraveContextMenuBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(tabs::features::kBraveSplitView);
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenuInWebContents(
      content::WebContents* web_contents,
      const GURL& unfiltered_url,
      const GURL& url,
      const std::u16string& link_text,
      blink::mojom::ContextMenuDataMediaType media_type,
      ui::mojom::MenuSourceType source_type) {
    content::ContextMenuParams params;
    params.media_type = media_type;
    params.unfiltered_link_url = unfiltered_url;
    params.link_url = url;
    params.src_url = url;
    params.link_text = link_text;
    params.page_url = web_contents->GetVisibleURL();
    params.source_type = source_type;
#if BUILDFLAG(IS_MAC)
    params.writing_direction_default = 0;
    params.writing_direction_left_to_right = 0;
    params.writing_direction_right_to_left = 0;
#endif
    auto menu = std::make_unique<TestRenderViewContextMenu>(
        *web_contents->GetPrimaryMainFrame(), params);
    menu->Init();
    return menu;
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenu(
      const GURL& unfiltered_url,
      const GURL& url,
      const std::u16string& link_text,
      blink::mojom::ContextMenuDataMediaType media_type,
      ui::mojom::MenuSourceType source_type) {
    return CreateContextMenuInWebContents(
        browser()->tab_strip_model()->GetActiveWebContents(), unfiltered_url,
        url, link_text, media_type, source_type);
  }

  std::unique_ptr<TestRenderViewContextMenu> CreateContextMenuMediaTypeNone(
      const GURL& unfiltered_url,
      const GURL& url) {
    return CreateContextMenu(unfiltered_url, url, std::u16string(),
                             blink::mojom::ContextMenuDataMediaType::kNone,
                             ui::mojom::MenuSourceType::kNone);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveContextMenuBrowserTest, OpenLinkInSplitView) {
  std::unique_ptr<TestRenderViewContextMenu> menu(
      CreateContextMenuMediaTypeNone(GURL("https://brave.com/"),
                                     GURL("https://brave.com/")));

  EXPECT_TRUE(
      menu->IsCommandIdEnabled(IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW));

  menu->ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW, 0);

  auto* model = static_cast<BraveTabStripModel*>(browser()->tab_strip_model());
  auto indices = model->GetTabIndicesForCommandAt(
      browser()->tab_strip_model()->active_index());

  EXPECT_TRUE(brave::IsTabsTiled(browser(), indices));
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
  EXPECT_TRUE(brave::IsTabsTiled(browser(), {0}));
  EXPECT_TRUE(brave::IsTabsTiled(browser(), {1}));

  // Check if "Open link in split view" is now got disabled.
  EXPECT_FALSE(
      menu->IsCommandIdEnabled(IDC_CONTENT_CONTEXT_OPENLINK_SPLIT_VIEW));
}
