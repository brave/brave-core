// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/ref_counted_memory.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "printing/buildflags/buildflags.h"
#include "ui/webui/mojo_bubble_web_ui_controller.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/untrusted_web_ui_controller.h"

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"
#include "components/printing/common/print.mojom.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

using printing::mojom::PrintPreviewUI;
#endif

namespace content {
class BrowserContext;
}

class Profile;

namespace ai_chat {
class AIChatUIPageHandler;
}  // namespace ai_chat

class AIChatUI : public ui::UntrustedWebUIController
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
    ,
                 public PrintPreviewUI {
#else
{
#endif

 public:
  explicit AIChatUI(content::WebUI* web_ui);
  AIChatUI(const AIChatUI&) = delete;
  AIChatUI& operator=(const AIChatUI&) = delete;
  ~AIChatUI() override;

  void BindInterface(
      mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver);

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  mojo::PendingAssociatedRemote<PrintPreviewUI> BindPrintPreviewUI();
  void DisconnectPrintPrieviewUI();
  bool IsBound() const;
  void SetPreviewUIId();
  std::optional<int32_t> GetPreviewUIId();
  void ClearPreviewUIId();
  void OnPrintPreviewRequest(int request_id);
#endif

  // Set by WebUIContentsWrapperT. MojoBubbleWebUIController provides default
  // implementation for this but we don't use it.
  void set_embedder(
      base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder) {
    embedder_ = embedder;
  }

  static constexpr std::string GetWebUIName() { return "AIChatPanel"; }

 private:
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // printing::mojo::PrintPreviewUI:
  void SetOptionsFromDocument(
      const printing::mojom::OptionsFromDocumentParamsPtr params,
      int32_t request_id) override;
  void DidPrepareDocumentForPreview(int32_t document_cookie,
                                    int32_t request_id) override;
  void DidPreviewPage(printing::mojom::DidPreviewPageParamsPtr params,
                      int32_t request_id) override;
  void MetafileReadyForPrinting(
      printing::mojom::DidPreviewDocumentParamsPtr params,
      int32_t request_id) override;
  void PrintPreviewFailed(int32_t document_cookie, int32_t request_id) override;
  void PrintPreviewCancelled(int32_t document_cookie,
                             int32_t request_id) override;
  void PrinterSettingsInvalid(int32_t document_cookie,
                              int32_t request_id) override;
  void DidGetDefaultPageLayout(
      printing::mojom::PageSizeMarginsPtr page_layout_in_points,
      const gfx::RectF& printable_area_in_points,
      bool all_pages_have_custom_size,
      bool all_pages_have_custom_orientation,
      int32_t request_id) override;
  void DidStartPreview(printing::mojom::DidStartPreviewParamsPtr params,
                       int32_t request_id) override;

  void OnPrepareForDocumentToPdfDone(
      int32_t request_id,
      printing::mojom::PrintCompositor::Status status);
  void OnCompositePdfPageDone(uint32_t page_index,
                              int32_t document_cookie,
                              int32_t request_id,
                              printing::mojom::PrintCompositor::Status status,
                              base::ReadOnlySharedMemoryRegion region);
  void OnCompositeToPdfDone(int document_cookie,
                            int32_t request_id,
                            printing::mojom::PrintCompositor::Status status,
                            base::ReadOnlySharedMemoryRegion region);
#endif

  std::unique_ptr<ai_chat::AIChatUIPageHandler> page_handler_;

  base::WeakPtr<ui::MojoBubbleWebUIController::Embedder> embedder_;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<content::WebContents> web_contents_ = nullptr;

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // unique id to avoid conflicts with other print preview UIs
  std::optional<int32_t> print_preview_ui_id_;
  mojo::AssociatedReceiver<PrintPreviewUI> receiver_{this};
#endif

  base::WeakPtrFactory<AIChatUI> weak_ptr_factory_{this};
  WEB_UI_CONTROLLER_TYPE_DECL();
};

class UntrustedChatUIConfig : public content::WebUIConfig {
 public:
  UntrustedChatUIConfig();
  ~UntrustedChatUIConfig() override = default;

  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;

  std::unique_ptr<content::WebUIController> CreateWebUIController(
      content::WebUI* web_ui,
      const GURL& url) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_H_
