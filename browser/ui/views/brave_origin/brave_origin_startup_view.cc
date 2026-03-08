/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_origin/brave_origin_startup_view.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/browser/ui/webui/brave_origin_startup/brave_origin_startup_ui.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

constexpr int kDialogWidth = 500;
constexpr int kDialogHeight = 600;

BraveOriginStartupView* g_startup_view = nullptr;

bool HasOriginSkuCredentials(PrefService* local_state) {
  const auto& skus_state = local_state->GetDict(skus::prefs::kSkusState);
  for (const auto [env_key, env_value] : skus_state) {
    if (!env_value.is_string()) {
      continue;
    }
    auto parsed =
        base::JSONReader::ReadDict(env_value.GetString(), base::JSON_PARSE_RFC);
    if (!parsed) {
      continue;
    }
    const auto* credentials = parsed->FindDict("credentials");
    if (!credentials) {
      continue;
    }
    const auto* items = credentials->FindDict("items");
    if (items && !items->empty()) {
      return true;
    }
  }
  return false;
}

}  // namespace

// static
bool BraveOriginStartupView::ShouldShowDialog(PrefService* local_state) {
  return !local_state->GetBoolean(brave_origin::kOriginPurchaseValidated) ||
         !HasOriginSkuCredentials(local_state);
}

// static
void BraveOriginStartupView::Show(
    base::OnceClosure on_complete,
    OpenExternalCallback open_external,
    base::RepeatingClosure attempt_exit,
    CreateProfilesCallback create_system_profile,
    CreateProfilesCallback create_default_profile) {
  if (g_startup_view) {
    return;
  }
  g_startup_view = new BraveOriginStartupView(
      std::move(on_complete), std::move(open_external), std::move(attempt_exit),
      std::move(create_system_profile), std::move(create_default_profile));
  g_startup_view->Display();
}

// static
void BraveOriginStartupView::Hide() {
  if (g_startup_view) {
    g_startup_view->GetWidget()->Close();
  }
}

// static
bool BraveOriginStartupView::IsShowing() {
  return g_startup_view != nullptr;
}

BraveOriginStartupView::BraveOriginStartupView(
    base::OnceClosure on_complete,
    OpenExternalCallback open_external,
    base::RepeatingClosure attempt_exit,
    CreateProfilesCallback create_system_profile,
    CreateProfilesCallback create_default_profile)
    : keep_alive_(KeepAliveOrigin::USER_MANAGER_VIEW,
                  KeepAliveRestartOption::DISABLED),
      on_complete_(std::move(on_complete)),
      open_external_(std::move(open_external)),
      attempt_exit_(std::move(attempt_exit)),
      create_system_profile_(std::move(create_system_profile)),
      create_default_profile_(std::move(create_default_profile)) {
  SetHasWindowSizeControls(false);
  SetTitle(l10n_util::GetStringUTF16(IDS_BRAVE_ORIGIN_STARTUP_TITLE));
}

BraveOriginStartupView::~BraveOriginStartupView() {
  if (contents_) {
    contents_->SetDelegate(nullptr);
  }
  if (g_startup_view == this) {
    g_startup_view = nullptr;
  }
}

void BraveOriginStartupView::Display() {
  BuildLayout();

  // Load both the system profile (for WebUI) and the default user profile
  // (for SKU service, which requires a regular profile).
  create_system_profile_.Run(
      base::BindOnce(&BraveOriginStartupView::OnSystemProfileCreated,
                     weak_ptr_factory_.GetWeakPtr()));
  create_default_profile_.Run(
      base::BindOnce(&BraveOriginStartupView::OnDefaultProfileCreated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveOriginStartupView::OnSystemProfileCreated(Profile* profile) {
  if (!profile) {
    // If profile creation fails, proceed without the dialog.
    if (on_complete_) {
      validated_ = true;
      std::move(on_complete_).Run();
    }
    return;
  }
  system_profile_ = profile;
  MaybeInit();
}

void BraveOriginStartupView::OnDefaultProfileCreated(Profile* profile) {
  // Store the default profile for SKU operations. If it fails, we'll
  // still show the dialog but SKU operations will report an error.
  default_profile_ = profile;
  MaybeInit();
}

void BraveOriginStartupView::MaybeInit() {
  // Wait until both profile callbacks have fired. Using a manual counter
  // instead of base::BarrierClosure because OnSystemProfileCreated has an
  // early-exit path that bypasses Init() entirely on failure.
  ++profiles_loaded_count_;
  if (profiles_loaded_count_ < 2) {
    return;
  }
  // System profile is required; default profile is optional.
  if (!system_profile_) {
    return;
  }
  Init(system_profile_);
}

void BraveOriginStartupView::Init(Profile* system_profile) {
  contents_ = content::WebContents::Create(
      content::WebContents::CreateParams(system_profile));
  contents_->SetDelegate(this);

  // Observe the WebContents to know when the page is loaded.
  content::WebContentsObserver::Observe(contents_.get());

  profile_keep_alive_ = std::make_unique<ScopedProfileKeepAlive>(
      system_profile->GetOriginalProfile(),
      ProfileKeepAliveOrigin::kProfilePickerView);

  web_view_->SetWebContents(contents_.get());
  contents_->GetController().LoadURL(
      GURL(kBraveOriginStartupURL), content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, std::string());

  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = this;
  params.bounds = gfx::Rect(kDialogWidth, kDialogHeight);
  widget_ = std::make_unique<views::Widget>();
  widget_->Init(std::move(params));

  GetWidget()->CenterWindow(gfx::Size(kDialogWidth, kDialogHeight));
  GetWidget()->Show();
}

void BraveOriginStartupView::BuildLayout() {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  web_view_ = AddChildView(std::make_unique<views::WebView>(nullptr));
}

void BraveOriginStartupView::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  // Only handle the main frame.
  if (render_frame_host != contents_->GetPrimaryMainFrame()) {
    return;
  }
  SetupWebUICallbacks();
}

void BraveOriginStartupView::SetupWebUICallbacks() {
  auto* web_ui = contents_->GetWebUI();
  if (!web_ui) {
    return;
  }

  auto* controller = web_ui->GetController()->GetAs<BraveOriginStartupUI>();
  if (!controller) {
    return;
  }

  controller->SetCallbacks(
      default_profile_.get(),
      base::BindRepeating(&BraveOriginStartupView::OpenBuyWindow,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&BraveOriginStartupView::CloseAndProceed,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveOriginStartupView::OpenBuyWindow() {
  // STAGING for unofficial builds; official builds always resolve to prod.
  std::string url =
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                    brave_domains::GetServicesDomain(
                        "account", brave_domains::ServicesEnvironment::STAGING),
                    "/?intent=checkout&product=origin"});
  GURL gurl(url);
  if (gurl.is_valid() && open_external_) {
    open_external_.Run(gurl);
  }
}

void BraveOriginStartupView::CloseAndProceed() {
  validated_ = true;
  if (on_complete_) {
    std::move(on_complete_).Run();
  }
  GetWidget()->Close();
}

content::WebContents* BraveOriginStartupView::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::NavigationHandle&)>
        navigation_handle_callback) {
  // Block all navigation away from the startup dialog.
  return nullptr;
}

content::WebContents* BraveOriginStartupView::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture,
    bool* was_blocked) {
  // Block all new windows/tabs from the startup dialog.
  if (was_blocked) {
    *was_blocked = true;
  }
  return nullptr;
}

void BraveOriginStartupView::WindowClosing() {
  if (!validated_) {
    // User closed without validating - exit browser. Post as async task
    // to avoid destroying objects while the window close is still processing.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
                                                             attempt_exit_);
  }
}

std::u16string BraveOriginStartupView::GetAccessibleWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_BRAVE_ORIGIN_STARTUP_TITLE);
}

gfx::Size BraveOriginStartupView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kDialogWidth, kDialogHeight);
}

bool BraveOriginStartupView::HandleKeyboardEvent(
    content::WebContents* source,
    const input::NativeWebKeyboardEvent& event) {
  return unhandled_keyboard_event_handler_.HandleKeyboardEvent(
      event, GetFocusManager());
}

BEGIN_METADATA(BraveOriginStartupView)
END_METADATA
