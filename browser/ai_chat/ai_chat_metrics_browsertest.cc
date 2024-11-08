/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"

namespace ai_chat {

class AIChatMetricsTest : public InProcessBrowserTest {
 public:
  AIChatMetricsTest() = default;
  AIChatMetricsTest(const AIChatMetricsTest&) = delete;
  AIChatMetricsTest& operator=(const AIChatMetricsTest&) = delete;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    content::ContextMenuParams params;
    params.is_editable = false;
    params.selection_text = u"some text";
    params.page_url = GURL("http://ai-test.local/");
    menu_ = std::make_unique<TestRenderViewContextMenu>(
        *browser()
             ->tab_strip_model()
             ->GetActiveWebContents()
             ->GetPrimaryMainFrame(),
        params);
    ai_chat_metrics_ =
        g_brave_browser_process->process_misc_metrics()->ai_chat_metrics();
  }

  void TearDownOnMainThread() override { menu_ = nullptr; }

 protected:
  base::HistogramTester histogram_tester_;
  std::unique_ptr<TestRenderViewContextMenu> menu_;
  raw_ptr<AIChatMetrics, DanglingUntriaged> ai_chat_metrics_;
};

IN_PROC_BROWSER_TEST_F(AIChatMetricsTest, ContextMenuActions) {
  menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT, /*event_flags=*/0);
  menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SUMMARIZE_TEXT, /*event_flags=*/0);
  menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_EXPLAIN, /*event_flags=*/0);
  histogram_tester_.ExpectTotalCount(kMostUsedContextMenuActionHistogramName,
                                     0);

  ai_chat_metrics_->RecordEnabled(
      true, true,
      base::BindLambdaForTesting(
          [&](mojom::Service::GetPremiumStatusCallback callback) {
            std::move(callback).Run(mojom::PremiumStatus::Active, nullptr);
          }));
  histogram_tester_.ExpectUniqueSample(kMostUsedContextMenuActionHistogramName,
                                       0, 1);

  menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_EXPLAIN, /*event_flags=*/0);
  menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_EXPLAIN, /*event_flags=*/0);
  histogram_tester_.ExpectBucketCount(kMostUsedContextMenuActionHistogramName,
                                      1, 1);

  for (int i = 0; i < 4; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_PARAPHRASE, /*event_flags=*/0);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextMenuActionHistogramName,
                                      2, 1);

  for (int i = 0; i < 5; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_CREATE_TAGLINE,
                          /*event_flags=*/0);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextMenuActionHistogramName,
                                      3, 1);

  for (int i = 0; i < 3; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_SHORT,
                          /*event_flags=*/0);
  }
  for (int i = 0; i < 3; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_CREATE_SOCIAL_MEDIA_COMMENT_LONG,
                          /*event_flags=*/0);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextMenuActionHistogramName,
                                      4, 1);

  for (int i = 0; i < 7; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_IMPROVE, /*event_flags=*/0);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextMenuActionHistogramName,
                                      5, 1);

  for (int i = 0; i < 4; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_ACADEMICIZE, /*event_flags=*/0);
  }
  for (int i = 0; i < 4; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_PROFESSIONALIZE,
                          /*event_flags=*/0);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextMenuActionHistogramName,
                                      6, 1);

  for (int i = 0; i < 5; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_SHORTEN, /*event_flags=*/0);
  }
  for (int i = 0; i < 4; i++) {
    menu_->ExecuteCommand(IDC_AI_CHAT_CONTEXT_EXPAND, /*event_flags=*/0);
  }
  histogram_tester_.ExpectBucketCount(kMostUsedContextMenuActionHistogramName,
                                      7, 1);
}

}  // namespace ai_chat
