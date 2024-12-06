/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/webcompat_reporter/webcompat_reporter_ui.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_browser_process.h"
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
#include "brave/components/webcompat_reporter/resources/grit/webcompat_reporter_generated_map.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/grit/brave_components_resources.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/gfx/codec/png_codec.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#endif

namespace webcompat_reporter {

namespace {

constexpr char kUISourceHistogramName[] = "Brave.Webcompat.UISource";
constexpr int kMaxScreenshotPixelCount = 1280 * 720;

std::string BoolToString(bool value) {
  return value ? "true" : "false";
}

}  // namespace

WebcompatReporterDOMHandler::WebcompatReporterDOMHandler(Profile* profile)
    : ui_task_runner_(base::SequencedTaskRunner::GetCurrentDefault()),
      pending_report_(mojom::ReportInfo::New()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  InitAdditionalParameters(profile);

  auto* browser = chrome::FindLastActiveWithProfile(profile);
  if (!browser) {
    return;
  }
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }
  render_widget_host_view_ = web_contents->GetTopLevelRenderWidgetHostView();
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
}

void WebcompatReporterDOMHandler::HandleCaptureScreenshot(
    const base::Value::List& args) {
  CHECK(render_widget_host_view_);
  CHECK_EQ(args.size(), 1u);

  AllowJavascript();

  auto output_size = render_widget_host_view_->GetVisibleViewportSize();
  auto original_area = output_size.GetArea();

  if (original_area > kMaxScreenshotPixelCount) {
    // Scale image down if it's too big
    float output_scale =
        std::sqrt(static_cast<float>(kMaxScreenshotPixelCount) / original_area);
    output_size = gfx::ScaleToRoundedSize(output_size, output_scale);
  }

  render_widget_host_view_->CopyFromSurface(
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

  auto* reporter_service =
      WebcompatReporterServiceFactory::GetServiceForContext(
          Profile::FromWebUI(web_ui()));
  if (reporter_service) {
    reporter_service->SubmitWebcompatReport(pending_report_->Clone());
  }
}

WebcompatReporterUI::WebcompatReporterUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui) {
  CreateAndAddWebUIDataSource(
      web_ui, kWebcompatReporterHost, kWebcompatReporterGenerated,
      kWebcompatReporterGeneratedSize, IDR_WEBCOMPAT_REPORTER_HTML);
  Profile* profile = Profile::FromWebUI(web_ui);

  web_ui->AddMessageHandler(
      std::make_unique<WebcompatReporterDOMHandler>(profile));
}

WebcompatReporterUI::~WebcompatReporterUI() = default;

}  // namespace webcompat_reporter
