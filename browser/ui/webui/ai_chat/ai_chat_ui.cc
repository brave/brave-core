// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"

#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/resources/page/grit/ai_chat_ui_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/printing/print_preview_data_service.h"
#include "chrome/browser/printing/print_view_manager_common.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "components/printing/browser/print_composite_client.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "printing/print_job_constants.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser.h"
#else
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#endif

using printing::PrintCompositeClient;

#if BUILDFLAG(IS_ANDROID)
namespace {
content::WebContents* GetActiveWebContents(content::BrowserContext* context) {
  auto tab_models = TabModelList::models();
  auto iter = base::ranges::find_if(
      tab_models, [](const auto& model) { return model->IsActiveModel(); });
  if (iter == tab_models.end()) {
    return nullptr;
  }

  auto* active_contents = (*iter)->GetActiveWebContents();
  if (!active_contents) {
    return nullptr;
  }
  DCHECK_EQ(active_contents->GetBrowserContext(), context);
  return active_contents;
}
}  // namespace
#endif

AIChatUI::AIChatUI(content::WebUI* web_ui)
    : ui::UntrustedWebUIController(web_ui),
      profile_(Profile::FromWebUI(web_ui)) {
  DCHECK(profile_);
  DCHECK(brave::IsRegularProfile(profile_));

  // Create a URLDataSource and add resources.
  content::WebUIDataSource* untrusted_source =
      content::WebUIDataSource::CreateAndAdd(
          web_ui->GetWebContents()->GetBrowserContext(), kChatUIURL);

  webui::SetupWebUIDataSource(
      untrusted_source,
      base::make_span(kAiChatUiGenerated, kAiChatUiGeneratedSize),
      IDR_CHAT_UI_HTML);

  untrusted_source->AddResourcePath("styles.css", IDR_CHAT_UI_CSS);

  for (const auto& str : ai_chat::GetLocalizedStrings()) {
    untrusted_source->AddString(
        str.name, brave_l10n::GetLocalizedResourceUTF16String(str.id));
  }

  base::Time last_accepted_disclaimer =
      profile_->GetOriginalProfile()->GetPrefs()->GetTime(
          ai_chat::prefs::kLastAcceptedDisclaimer);

  untrusted_source->AddBoolean("hasAcceptedAgreement",
                               !last_accepted_disclaimer.is_null());

#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  constexpr bool kIsMobile = true;
#else
  constexpr bool kIsMobile = false;
#endif

  untrusted_source->AddBoolean("isMobile", kIsMobile);

  untrusted_source->AddBoolean(
      "hasUserDismissedPremiumPrompt",
      profile_->GetOriginalProfile()->GetPrefs()->GetBoolean(
          ai_chat::prefs::kUserDismissedPremiumPrompt));

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ScriptSrc,
      "script-src 'self' chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::StyleSrc,
      "style-src 'self' 'unsafe-inline' chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src 'self' blob: chrome-untrusted://resources;");
  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FontSrc,
      "font-src 'self' data: chrome-untrusted://resources;");

  untrusted_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes, "trusted-types default;");
}

AIChatUI::~AIChatUI() {
  ClearPreviewUIId();
}

void AIChatUI::BindInterface(
    mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver) {
  // We call ShowUI() before creating the PageHandler object so that
  // the WebContents is added to a Browser which we can get a reference
  // to and provide to the PageHandler.
  if (embedder_) {
    embedder_->ShowUI();
  }

#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<Browser> browser =
      ai_chat::GetBrowserForWebContents(web_ui()->GetWebContents());
  DCHECK(browser);
  TabStripModel* tab_strip_model = browser->tab_strip_model();
  DCHECK(tab_strip_model);
  web_contents_ = tab_strip_model->GetActiveWebContents();
#else
  web_contents_ = GetActiveWebContents(profile_);
#endif
  if (web_contents_ == web_ui()->GetWebContents()) {
    web_contents_ = nullptr;
  }
  page_handler_ = std::make_unique<ai_chat::AIChatUIPageHandler>(
      this, web_ui()->GetWebContents(), web_contents_.get(), profile_,
      std::move(receiver));
}

mojo::PendingAssociatedRemote<PrintPreviewUI> AIChatUI::BindPrintPreviewUI() {
  return receiver_.BindNewEndpointAndPassRemote();
}

void AIChatUI::DisconnectPrintPrieviewUI() {
  receiver_.reset();
}

bool AIChatUI::IsBound() const {
  return receiver_.is_bound();
}

void AIChatUI::SetPreviewUIId() {
  DCHECK(!print_preview_ui_id_);
  print_preview_ui_id_ =
      printing::PrintPreviewUI::GetPrintPreviewUIIdMap().Add(this);
  printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap()
      [*print_preview_ui_id_] = -1;
}

std::optional<int32_t> AIChatUI::GetPreviewUIId() {
  return print_preview_ui_id_;
}

void AIChatUI::ClearPreviewUIId() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!print_preview_ui_id_) {
    return;
  }

  DisconnectPrintPrieviewUI();
  PrintPreviewDataService::GetInstance()->RemoveEntry(*print_preview_ui_id_);
  printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap().erase(
      *print_preview_ui_id_);
  printing::PrintPreviewUI::GetPrintPreviewUIIdMap().Remove(
      *print_preview_ui_id_);
  print_preview_ui_id_.reset();
}

void AIChatUI::OnPrintPreviewRequest(int request_id) {
  printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap()
      [*print_preview_ui_id_] = request_id;
}

void AIChatUI::SetOptionsFromDocument(
    const printing::mojom::OptionsFromDocumentParamsPtr params,
    int32_t request_id) {}

void AIChatUI::DidPrepareDocumentForPreview(int32_t document_cookie,
                                            int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id;
  // For case of print preview, page metafile is used to composite into
  // the document PDF at same time.  Need to indicate that this scenario
  // is at play for the compositor.
  auto* client = PrintCompositeClient::FromWebContents(web_contents_);
  DCHECK(client);
  if (client->GetIsDocumentConcurrentlyComposited(document_cookie)) {
    return;
  }

  content::RenderFrameHost* render_frame_host =
      printing::GetFrameToPrint(web_contents_.get());
  // |render_frame_host| could be null when the print preview dialog is closed.
  if (!render_frame_host) {
    return;
  }

  client->PrepareToCompositeDocument(
      document_cookie, render_frame_host,
      PrintCompositeClient::GetDocumentType(),
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&AIChatUI::OnPrepareForDocumentToPdfDone,
                         weak_ptr_factory_.GetWeakPtr(), request_id),
          printing::mojom::PrintCompositor::Status::kCompositingFailure));
}
void AIChatUI::DidPreviewPage(printing::mojom::DidPreviewPageParamsPtr params,
                              int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id;
  uint32_t page_index = params->page_index;
  const printing::mojom::DidPrintContentParams& content = *params->content;
  if (page_index == printing::kInvalidPageIndex ||
      !content.metafile_data_region.IsValid()) {
    return;
  }

  auto* client = PrintCompositeClient::FromWebContents(web_contents_);
  DCHECK(client);

  content::RenderFrameHost* render_frame_host =
      printing::GetFrameToPrint(web_contents_.get());
  if (!render_frame_host) {
    DLOG(ERROR) << "No render frame host for print preview";
    return;
  }

  client->CompositePage(
      params->document_cookie, render_frame_host, content,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(&AIChatUI::OnCompositePdfPageDone,
                         weak_ptr_factory_.GetWeakPtr(), page_index,
                         params->document_cookie, request_id),
          printing::mojom::PrintCompositor::Status::kCompositingFailure,
          base::ReadOnlySharedMemoryRegion()));
}
void AIChatUI::MetafileReadyForPrinting(
    printing::mojom::DidPreviewDocumentParamsPtr params,
    int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id;
  auto callback = base::BindOnce(&AIChatUI::OnCompositeToPdfDone,
                                 weak_ptr_factory_.GetWeakPtr(),
                                 params->document_cookie, request_id);

  // Page metafile is used to composite into the document at same time.
  // Need to provide particulars of how many pages are required before
  // document will be completed.
  auto* client = PrintCompositeClient::FromWebContents(web_contents_);
  client->FinishDocumentComposition(
      params->document_cookie, params->expected_pages_count,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          std::move(callback),
          printing::mojom::PrintCompositor::Status::kCompositingFailure,
          base::ReadOnlySharedMemoryRegion()));
}
void AIChatUI::PrintPreviewFailed(int32_t document_cookie, int32_t request_id) {
  DLOG(ERROR) << __func__ << ": id=" << request_id;
  if (print_preview_ui_id_) {
    printing::PrintPreviewUI::GetPrintPreviewUIRequestIdMap()
        [*print_preview_ui_id_] = -1;
  }
}
void AIChatUI::PrintPreviewCancelled(int32_t document_cookie,
                                     int32_t request_id) {
  DLOG(ERROR) << __func__ << ": id=" << request_id;
}
void AIChatUI::PrinterSettingsInvalid(int32_t document_cookie,
                                      int32_t request_id) {
  DLOG(ERROR) << __func__ << ": id=" << request_id;
}

void AIChatUI::DidGetDefaultPageLayout(
    printing::mojom::PageSizeMarginsPtr page_layout_in_points,
    const gfx::RectF& printable_area_in_points,
    bool all_pages_have_custom_size,
    bool all_pages_have_custom_orientation,
    int32_t request_id) {}

void AIChatUI::DidStartPreview(printing::mojom::DidStartPreviewParamsPtr params,
                               int32_t request_id) {
  DVLOG(3) << __func__ << ": id=" << request_id;
}

void AIChatUI::OnPrepareForDocumentToPdfDone(
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status) {
  DVLOG(3) << __func__ << ": id=" << request_id << " , status=" << status;
}

void AIChatUI::OnCompositePdfPageDone(
    uint32_t page_index,
    int document_cookie,
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DVLOG(3) << __func__ << ": id=" << request_id << " , status=" << status;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(print_preview_ui_id_);

  PrintPreviewDataService::GetInstance()->SetDataEntry(
      *print_preview_ui_id_, page_index,
      base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
}

void AIChatUI::OnCompositeToPdfDone(
    int document_cookie,
    int32_t request_id,
    printing::mojom::PrintCompositor::Status status,
    base::ReadOnlySharedMemoryRegion region) {
  DVLOG(3) << __func__ << ": id=" << request_id << " , status=" << status;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(print_preview_ui_id_);
  PrintPreviewDataService::GetInstance()->SetDataEntry(
      *print_preview_ui_id_, printing::COMPLETE_PREVIEW_DOCUMENT_INDEX,
      base::RefCountedSharedMemoryMapping::CreateFromWholeRegion(region));
  page_handler_->OnPreviewReady();
}

std::unique_ptr<content::WebUIController>
UntrustedChatUIConfig::CreateWebUIController(content::WebUI* web_ui,
                                             const GURL& url) {
  return std::make_unique<AIChatUI>(web_ui);
}

bool UntrustedChatUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return ai_chat::IsAIChatEnabled(
             user_prefs::UserPrefs::Get(browser_context)) &&
         brave::IsRegularProfile(browser_context);
}

UntrustedChatUIConfig::UntrustedChatUIConfig()
    : WebUIConfig(content::kChromeUIUntrustedScheme, kChatUIHost) {}

WEB_UI_CONTROLLER_TYPE_IMPL(AIChatUI)
