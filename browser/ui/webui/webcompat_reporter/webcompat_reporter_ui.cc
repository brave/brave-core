/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_ui.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_dialog.h"
#include "brave/browser/webcompat_reporter/webcompat_reporter_service_factory.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/webcompat_reporter/browser/fields.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_utils.h"
#include "brave/components/webcompat_reporter/resources/grit/webcompat_reporter_generated_map.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "components/grit/brave_components_resources.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#endif

namespace webcompat_reporter {

namespace {

constexpr char kUISourceHistogramName[] = "Brave.Webcompat.UISource";
constexpr int kMaxScreenshotPixelCount = 1280 * 720;
constexpr char kGetViewPortSizeParamName[] = "height";
constexpr char kOnViewPortSizeChangedEventName[] = "onViewPortSizeChanged";

content::WebContents* GetActiveWebContents() {
  const auto* browser = BrowserList::GetInstance()->GetLastActive();
  if (!browser) {
    return nullptr;
  }
  return browser->tab_strip_model()->GetActiveWebContents();
}

const std::optional<gfx::Rect> GetContainerBounds(content::WebUI* web_ui) {
  if (!web_ui) {
    auto* web_contents = GetActiveWebContents();
    return web_contents ? std::make_optional(web_contents->GetContainerBounds())
                        : std::nullopt;
  }

  return web_ui->GetWebContents()->GetContainerBounds();
}

views::Widget* GetBrowserWidget() {
  const auto* browser = BrowserList::GetInstance()->GetLastActive();
  if (!browser) {
    return nullptr;
  }

  auto* widget = views::Widget::GetWidgetForNativeWindow(
      browser->window()->GetNativeWindow());
  if (!widget) {
    return nullptr;
  }

  return widget->GetPrimaryWindowWidget();
}

const std::optional<int> GetDlgMaxHeight(content::WebUI* web_ui,
                                         const views::Widget* browser_widget) {
  DCHECK(browser_widget);
  if (!browser_widget) {
    return std::nullopt;
  }
  const auto modal_dlg_bounds = GetContainerBounds(web_ui);
  if (!modal_dlg_bounds) {
    return std::nullopt;
  }

  const auto browser_wnd_bounds =
      browser_widget->client_view()->GetBoundsInScreen();

  return browser_wnd_bounds.bottom() - modal_dlg_bounds->y();
}

}  // namespace

WebcompatReporterDOMHandler::WebcompatReporterDOMHandler(Profile* profile)
    : reporter_service_(
          WebcompatReporterServiceFactory::GetServiceForContext(profile)),
      pref_service_(profile->GetPrefs()),
      ui_task_runner_(base::SequencedTaskRunner::GetCurrentDefault()),
      pending_report_(mojom::ReportInfo::New()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  InitAdditionalParameters(profile);
}

void WebcompatReporterDOMHandler::InitAdditionalParameters(Profile* profile) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  brave_vpn::BraveVpnService* vpn_service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(profile);
  if (vpn_service != nullptr) {
    pending_report_->brave_vpn_connected =
        BoolToString(vpn_service->IsConnected());
  }
#endif

  PrefService* profile_prefs = profile->GetPrefs();
  pending_report_->languages =
      profile_prefs->GetString(language::prefs::kAcceptLanguages);
  pending_report_->language_farbling = BoolToString(
      profile_prefs->GetBoolean(brave_shields::prefs::kReduceLanguageEnabled));
  pending_report_->channel = brave::GetChannelName();
}

WebcompatReporterDOMHandler::~WebcompatReporterDOMHandler() = default;

base::WeakPtr<WebcompatReporterDOMHandler>
WebcompatReporterDOMHandler::AsWeekPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void WebcompatReporterDOMHandler::OnWindowResize(const int& height) {
  AllowJavascript();
  base::Value::Dict event_data;
  event_data.Set(kGetViewPortSizeParamName, height);
  FireWebUIListener(kOnViewPortSizeChangedEventName, event_data);
}

void WebcompatReporterDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.submitReport",
      base::BindRepeating(&WebcompatReporterDOMHandler::HandleSubmitReport,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.captureScreenshot",
      base::BindRepeating(&WebcompatReporterDOMHandler::HandleCaptureScreenshot,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.getCapturedScreenshot",
      base::BindRepeating(
          &WebcompatReporterDOMHandler::HandleGetCapturedScreenshot,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.clearScreenshot",
      base::BindRepeating(&WebcompatReporterDOMHandler::HandleClearScreenshot,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "webcompat_reporter.init",
      base::BindRepeating(&WebcompatReporterDOMHandler::HandleInit,
                          base::Unretained(this)));
}

void WebcompatReporterDOMHandler::HandleCaptureScreenshot(
    const base::Value::List& args) {
  auto* render_widget_host_view =
      web_ui()->GetWebContents()->GetTopLevelRenderWidgetHostView();
  CHECK(render_widget_host_view);
  CHECK_EQ(args.size(), 1u);

  AllowJavascript();

  auto output_size = render_widget_host_view->GetVisibleViewportSize();
  auto original_area = output_size.GetArea();

  if (original_area > kMaxScreenshotPixelCount) {
    // Scale image down if it's too big
    float output_scale =
        std::sqrt(static_cast<float>(kMaxScreenshotPixelCount) / original_area);
    output_size = gfx::ScaleToRoundedSize(output_size, output_scale);
  }

  render_widget_host_view->CopyFromSurface(
      {}, output_size,
      base::BindOnce(
          [](base::WeakPtr<WebcompatReporterDOMHandler> handler,
             scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
             base::Value callback_id, const SkBitmap& bitmap) {
            ui_task_runner->PostTask(
                FROM_HERE,
                base::BindOnce(&WebcompatReporterDOMHandler::
                                   HandleCapturedScreenshotBitmap,
                               handler, bitmap, std::move(callback_id)));
          },
          weak_ptr_factory_.GetWeakPtr(), ui_task_runner_, args[0].Clone()));
}

void WebcompatReporterDOMHandler::HandleCapturedScreenshotBitmap(
    SkBitmap bitmap,
    base::Value callback_id) {
  if (bitmap.drawsNothing()) {
    LOG(ERROR) << "Failed to capture screenshot for webcompat report via "
                  "CopyFromSurface";
    RejectJavascriptCallback(callback_id, {});
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&gfx::PNGCodec::EncodeBGRASkBitmap, bitmap, false),
      base::BindOnce(&WebcompatReporterDOMHandler::HandleEncodedScreenshotPNG,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback_id)));
}

void WebcompatReporterDOMHandler::HandleEncodedScreenshotPNG(
    base::Value callback_id,
    std::optional<std::vector<unsigned char>> encoded_png) {
  if (!encoded_png) {
    LOG(ERROR) << "Failed to encode screenshot to PNG for webcompat report";
    RejectJavascriptCallback(callback_id, {});
    return;
  }
  pending_report_->screenshot_png = encoded_png;
  ResolveJavascriptCallback(callback_id, {});
}

void WebcompatReporterDOMHandler::HandleGetCapturedScreenshot(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1u);

  AllowJavascript();

  if (!pending_report_->screenshot_png) {
    RejectJavascriptCallback(args[0], {});
    return;
  }

  auto screenshot_b64 = base::Base64Encode(*pending_report_->screenshot_png);
  ResolveJavascriptCallback(args[0], screenshot_b64);
}

void WebcompatReporterDOMHandler::HandleInit(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1u);

  AllowJavascript();

  const auto max_height = GetDlgMaxHeight(nullptr, GetBrowserWidget());
  if (!max_height) {
    RejectJavascriptCallback(args[0], {});
    return;
  }

  base::Value::Dict event_data;
  event_data.Set(kGetViewPortSizeParamName, *max_height);
  ResolveJavascriptCallback(args[0], event_data);
}

void WebcompatReporterDOMHandler::HandleClearScreenshot(
    const base::Value::List& args) {
  pending_report_->screenshot_png = std::nullopt;
}

void WebcompatReporterDOMHandler::HandleSubmitReport(
    const base::Value::List& args) {
  DCHECK_EQ(args.size(), 1U);
  if (!args[0].is_dict()) {
    return;
  }

  const base::Value::Dict& submission_args = args[0].GetDict();

  const std::string* url_arg = submission_args.FindString(kSiteURLField);
  const std::string* ad_block_setting_arg =
      submission_args.FindString(kAdBlockSettingField);
  const std::string* fp_block_setting_arg =
      submission_args.FindString(kFPBlockSettingField);
  const base::Value* details_arg = submission_args.Find(kDetailsField);
  const base::Value* contact_arg = submission_args.Find(kContactField);
  pending_report_->shields_enabled = BoolToString(
      submission_args.FindBool(kShieldsEnabledField).value_or(false));

  const auto ui_source_int = submission_args.FindInt(kUISourceField);
  if (ui_source_int) {
    UISource ui_source = static_cast<UISource>(*ui_source_int);
    UMA_HISTOGRAM_ENUMERATION(kUISourceHistogramName, ui_source);
  }

  if (url_arg != nullptr) {
    pending_report_->report_url = *url_arg;
  }
  if (ad_block_setting_arg != nullptr) {
    pending_report_->ad_block_setting = *ad_block_setting_arg;
  }
  if (fp_block_setting_arg != nullptr) {
    pending_report_->fp_block_setting = *fp_block_setting_arg;
  }
  if (details_arg != nullptr && details_arg->is_string()) {
    pending_report_->details = details_arg->GetString();
  }
  if (contact_arg != nullptr && contact_arg->is_string()) {
    pending_report_->contact = contact_arg->GetString();
  }

  if (reporter_service_) {
    reporter_service_->SubmitWebcompatReport(pending_report_->Clone());
  }
}

WebcompatReporterUI::WebcompatReporterUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, kWebcompatReporterHost,
                              kWebcompatReporterGenerated,
                              IDR_WEBCOMPAT_REPORTER_HTML);
  auto* profile = Profile::FromWebUI(web_ui);

  auto webcompat_reporter_handler =
      std::make_unique<WebcompatReporterDOMHandler>(profile);

  webcompat_reporter_handler_ = webcompat_reporter_handler->AsWeekPtr();

  web_ui->AddMessageHandler(std::move(webcompat_reporter_handler));

  if (auto* widget = GetBrowserWidget()) {
    observed_windows_.AddObservation(widget);
  }
}

WebcompatReporterUI::~WebcompatReporterUI() = default;

void WebcompatReporterUI::OnWidgetBoundsChanged(views::Widget* widget,
                                                const gfx::Rect& new_bounds) {
  DCHECK(widget);
  DCHECK(webcompat_reporter_handler_);
  if (!webcompat_reporter_handler_ || !widget) {
    return;
  }

  if (const auto max_height = GetDlgMaxHeight(web_ui(), widget)) {
    webcompat_reporter_handler_->OnWindowResize(*max_height);
  }
}

}  // namespace webcompat_reporter
