/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_UI_H_

#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom-forward.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace content {
class RenderWidgetHostView;
class WebUI;
}  // namespace content

namespace webcompat_reporter {

class WebcompatReporterUI;
class WebcompatReporterService;

class WebcompatReporterUIConfig
    : public content::DefaultWebUIConfig<WebcompatReporterUI> {
 public:
  WebcompatReporterUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kWebcompatReporterHost) {}
};

class WebcompatReporterDOMHandler : public content::WebUIMessageHandler {
 public:
  explicit WebcompatReporterDOMHandler(Profile* profile);
  WebcompatReporterDOMHandler(const WebcompatReporterDOMHandler&) = delete;
  WebcompatReporterDOMHandler& operator=(const WebcompatReporterDOMHandler&) =
      delete;
  ~WebcompatReporterDOMHandler() override;

  void OnWindowResize(const int& height);
  base::WeakPtr<WebcompatReporterDOMHandler> AsWeekPtr();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void InitAdditionalParameters(Profile* profile);

  void HandleCaptureScreenshot(const base::Value::List& args);
  void HandleCapturedScreenshotBitmap(SkBitmap bitmap, base::Value callback_id);
  void HandleEncodedScreenshotPNG(
      base::Value callback_id,
      std::optional<std::vector<unsigned char>> encoded_png);

  void HandleGetCapturedScreenshot(const base::Value::List& args);
  void HandleClearScreenshot(const base::Value::List& args);

  void HandleSubmitReport(const base::Value::List& args);
  void HandleInit(const base::Value::List& args);

  raw_ptr<WebcompatReporterService> reporter_service_ = nullptr;
  raw_ptr<PrefService> pref_service_ = nullptr;
  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;

  mojom::ReportInfoPtr pending_report_;

  base::WeakPtrFactory<WebcompatReporterDOMHandler> weak_ptr_factory_{this};
};

class WebcompatReporterUI : public ConstrainedWebDialogUI,
                            public views::WidgetObserver {
 public:
  explicit WebcompatReporterUI(content::WebUI* web_ui);
  WebcompatReporterUI(const WebcompatReporterUI&) = delete;
  WebcompatReporterUI& operator=(const WebcompatReporterUI&) = delete;
  ~WebcompatReporterUI() override;

  // views::WidgetObserver
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;

 private:
  base::WeakPtr<WebcompatReporterDOMHandler> webcompat_reporter_handler_;
  base::ScopedMultiSourceObservation<views::Widget, views::WidgetObserver>
      observed_windows_{this};
};

}  // namespace webcompat_reporter

#endif  // BRAVE_BROWSER_UI_WEBUI_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_UI_H_
