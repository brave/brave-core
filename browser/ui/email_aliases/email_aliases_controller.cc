/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/email_aliases/email_aliases_controller.h"

#include <memory>
#include <optional>

#include "base/check_is_test.h"
#include "brave/browser/ui/webui/email_aliases/email_aliases_panel_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/email_aliases/constants.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "third_party/blink/public/mojom/forms/form_control_type.mojom-shared.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/gurl.h"

namespace {
inline constexpr char kEmailAliasesPanelURL[] = "chrome://email-aliases.panel/";
inline constexpr char kEmailAliasesPromoURL[] = "chrome://email-aliases.promo/";

constexpr int kDialogWidth = 512;
constexpr gfx::Size kDialogMinSize(kDialogWidth, 336);
constexpr gfx::Size kDialogMaxSize(kDialogWidth, 794);

bool g_autoclose_bubble_for_testing = false;

}  // namespace

namespace email_aliases {

namespace {

class EmailAliasesDialogDelegate
    : public ui::WebDialogDelegate,
      email_aliases::mojom::EmailAliasesPanelHandler,
      content::WebContentsObserver {
 public:
  ~EmailAliasesDialogDelegate() override = default;

  static ConstrainedWebDialogDelegate* Show(
      EmailAliasesController& controller,
      content::WebContents* initiator,
      content::GlobalRenderFrameHostId field_render_frame_host_id,
      uint64_t field_renderer_id) {
    return ShowConstrainedWebDialogWithAutoResize(
        initiator->GetBrowserContext(),
        base::WrapUnique(new EmailAliasesDialogDelegate(
            controller, GURL(kEmailAliasesPanelURL), field_render_frame_host_id, field_renderer_id)),
        initiator, kDialogMinSize, kDialogMaxSize);
  }
  static ConstrainedWebDialogDelegate* ShowPromo(
      EmailAliasesController& controller,
      content::WebContents* initiator) {
    return ShowConstrainedWebDialogWithAutoResize(
        initiator->GetBrowserContext(),
        base::WrapUnique(new EmailAliasesDialogDelegate(
            controller, GURL(kEmailAliasesPromoURL), content::GlobalRenderFrameHostId(), 0)),
        initiator, kDialogMinSize, kDialogMaxSize);
  }
 private:
  EmailAliasesDialogDelegate(
      EmailAliasesController& controller,
      const GURL& dialog_content_url,
      content::GlobalRenderFrameHostId field_render_frame_host_id,
      uint64_t field_renderer_id)
      : email_aliases_controller_(controller),
        field_render_frame_host_id_(field_render_frame_host_id),
        field_renderer_id_(field_renderer_id) {
    set_delete_on_close(false);
    set_dialog_content_url(dialog_content_url);
    set_show_dialog_title(false);
    set_close_dialog_on_escape(true);
    set_can_close(true);
  }

  // ui::WebDialogDelegate:
  void OnDialogShown(content::WebUI* webui) override {
    if (auto* ui = webui->GetController()->GetAs<EmailAliasesPanelUI>()) {
      ui->SetHandlerDelegate(this);
      Observe(webui->GetWebContents());
    }
  }

  // email_aliases::mojom::EmailAliasesPanelHandler:
  void OnAliasCreated(const std::string& email) override {
    Close();

    if (email.empty()) {
      return;
    }
    auto* field_render_frame =
        content::RenderFrameHost::FromID(field_render_frame_host_id_);
    if (!field_render_frame) {
      return;
    }
    auto* autofill_driver =
        autofill::ContentAutofillDriver::GetForRenderFrameHost(
            field_render_frame);
    if (!autofill_driver) {
      return;
    }
    autofill_driver->GetAutofillAgent()->ApplyFieldAction(
        autofill::mojom::FieldActionType::kReplaceAll,
        autofill::mojom::ActionPersistence::kFill,
        autofill::FieldRendererId(field_renderer_id_),
        base::UTF8ToUTF16(email));
  }

  void OnManageAliases() override {
    Close();
    email_aliases_controller_->OpenSettingsPage();
  }

  void OnCancelAliasCreation() override { Close(); }

  // content::WebContentsObserver:
  void PrimaryMainFrameRenderProcessGone(base::TerminationStatus) override {
    Close();
  }

  void OnWebContentsLostFocus(
      content::RenderWidgetHost* render_widget_host) override {
    if (!g_autoclose_bubble_for_testing) {
      Close();
    } else {
      CHECK_IS_TEST();
    }
  }

  void Close() {
    Observe(nullptr);
    email_aliases_controller_->CloseBubble();
  }

  raw_ref<EmailAliasesController> email_aliases_controller_;
  content::GlobalRenderFrameHostId field_render_frame_host_id_;
  uint64_t field_renderer_id_ = 0;
};

}  // namespace

EmailAliasesController::EmailAliasesController(
    BrowserView* browser_view,
    EmailAliasesService* email_aliases_service)
    : browser_view_(browser_view),
      email_aliases_service_(email_aliases_service) {
  CHECK(browser_view_);
  CHECK(email_aliases_service_.get());
}

EmailAliasesController::~EmailAliasesController() {
  CloseBubble();
}

bool EmailAliasesController::IsAvailableFor(
    const content::ContextMenuParams& params) const {
  if (!params.form_control_type) {
    return false;
  }

  using blink::mojom::FormControlType;
  return params.is_content_editable_for_autofill ||
         params.form_control_type == FormControlType::kInputEmail ||
         params.form_control_type == FormControlType::kInputText;
}

void EmailAliasesController::ShowBubble(
    content::WebContents* initiator,
    content::RenderFrameHost* render_frame,
    uint64_t field_renderer_id,
    std::optional<SettingsPageMethod> method) {
  if (!email_aliases_service_->IsAuthenticated()) {
    return OpenSettingsPage(method, initiator, render_frame);
  }

  CloseBubble();

  bubble_ = EmailAliasesDialogDelegate::Show(
      *this, initiator, render_frame?render_frame->GetGlobalId():content::GlobalRenderFrameHostId(), field_renderer_id);
  bubble_->GetWebDialogDelegate()->RegisterOnDialogClosedCallback(
      base::BindOnce(&EmailAliasesController::OnBubbleClosed,
                     weak_factory_.GetWeakPtr()));
}

void EmailAliasesController::CloseBubble() {
  if (!bubble_) {
    return;
  }
  bubble_->OnDialogCloseFromWebUI();
  bubble_->GetWebDialogDelegate()->OnDialogClosed({});
}

void EmailAliasesController::OpenSettingsPage(
    std::optional<SettingsPageMethod> method,
    content::WebContents* initiator,
    content::RenderFrameHost* render_frame) {
  if (method) {
    email_aliases_service_->metrics().RecordSettingsPageNavigation(*method);
  }
LOG(INFO) << "[EA] " << "\r\nStack:\r\n" << base::debug::StackTrace();
LOG(INFO) << "[EA] EmailAliasesController::OpenSettingsPage: IsPromoShown: " << email_aliases_service_->IsPromoShown() << " initiator: " << (initiator ? "true" : "false");

  if (!email_aliases_service_->IsPromoShown() && initiator) {
    LOG(INFO) << "[EA] EmailAliasesController::OpenSettingsPage: Show promo dialog";
    // TODO Here we have to initiate the showing of the promo dialog and as
    // callback parameter add there opening of the EmailAliases settings page
    ShowPromoBubble(initiator);

    
  } else {
    ShowSingletonTabOverwritingNTP(browser_view_->browser(),
                                   GURL(kEmailAliasesSettingsURL));
  }
}

content::WebContents* EmailAliasesController::GetBubbleForTesting() {
  if (!bubble_) {
    return nullptr;
  }
  return bubble_->GetWebContents();
}

// static
void EmailAliasesController::DisableAutoCloseBubbleForTesting(
    bool disale_autoclose) {
  g_autoclose_bubble_for_testing = disale_autoclose;
}

void EmailAliasesController::OnBubbleClosed(const std::string&) {
  bubble_ = nullptr;
}

void EmailAliasesController::ShowPromoBubble(content::WebContents* initiator) {
  CloseBubble();

  bubble_ = EmailAliasesDialogDelegate::ShowPromo(*this, initiator);
  bubble_->GetWebDialogDelegate()->RegisterOnDialogClosedCallback(
      base::BindOnce(&EmailAliasesController::OnPromoBubbleClosed,
                     weak_factory_.GetWeakPtr()));
}

void EmailAliasesController::OnPromoBubbleClosed(const std::string&) {
  OnBubbleClosed({});
  // TODO Then just set the promo shown prefs to true
  //email_aliases_service_->SetPromoShown();
}

}  // namespace email_aliases
