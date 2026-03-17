/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_origin/brave_origin_startup_view.h"

#include <utility>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/browser/ui/webui/brave_origin_startup/brave_origin_startup_ui.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/keep_alive/profile_keep_alive_types.h"
#include "chrome/browser/profiles/keep_alive/scoped_profile_keep_alive.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/content_switches.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {

constexpr int kDialogWidth = 500;
constexpr int kDialogHeight = 450;

BraveOriginStartupView* g_startup_view = nullptr;
std::optional<bool> g_should_show_dialog_override;

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
  if (g_should_show_dialog_override.has_value()) {
    return *g_should_show_dialog_override;
  }

  // Skip the dialog when running under test infrastructure.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kTestType)) {
    return false;
  }

#if BUILDFLAG(IS_LINUX)
  if (local_state->GetBoolean(brave_origin::kOriginFreeTierAccepted)) {
    return false;
  }
#endif

  return !local_state->GetBoolean(brave_origin::kOriginPurchaseValidated) ||
         !HasOriginSkuCredentials(local_state);
}

// static
void BraveOriginStartupView::SetShouldShowDialogForTesting(  // IN-TEST
    std::optional<bool> override) {
  g_should_show_dialog_override = override;
}

// static
void BraveOriginStartupView::Show(base::OnceClosure on_complete,
                                  std::unique_ptr<Delegate> delegate) {
  if (g_startup_view) {
    return;
  }
  g_startup_view =
      new BraveOriginStartupView(std::move(on_complete), std::move(delegate));
  g_startup_view->Display();
}

// static
void BraveOriginStartupView::Hide() {
  if (g_startup_view && g_startup_view->GetWidget()) {
    g_startup_view->GetWidget()->Close();
  }
}

// static
bool BraveOriginStartupView::IsShowing() {
  return g_startup_view != nullptr;
}

// static
void BraveOriginStartupView::ValidateForTesting() {  // IN-TEST
  if (g_startup_view && g_startup_view->GetWidget()) {
    g_startup_view->CloseAndProceed();
  }
}

BraveOriginStartupView::BraveOriginStartupView(
    base::OnceClosure on_complete,
    std::unique_ptr<Delegate> delegate)
    : keep_alive_(
          std::make_unique<ScopedKeepAlive>(KeepAliveOrigin::USER_MANAGER_VIEW,
                                            KeepAliveRestartOption::DISABLED)),
      delegate_(std::move(delegate)),
      on_complete_(std::move(on_complete)) {
  SetHasWindowSizeControls(false);
  SetTitle(IDS_BRAVE_ORIGIN_STARTUP_TITLE);
}

BraveOriginStartupView::~BraveOriginStartupView() {
  if (g_startup_view == this) {
    g_startup_view = nullptr;
  }
}

void BraveOriginStartupView::Display() {
  // Load both the system profile (for WebUI) and the default user profile
  // (for SKU service, which requires a regular profile).
  delegate_->CreateSystemProfile(
      base::BindOnce(&BraveOriginStartupView::OnSystemProfileCreated,
                     weak_ptr_factory_.GetWeakPtr()));
  delegate_->CreateDefaultProfile(
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
  profile_keep_alive_ = std::make_unique<ScopedProfileKeepAlive>(
      system_profile->GetOriginalProfile(),
      ProfileKeepAliveOrigin::kProfilePickerView);

  web_view_ = std::make_unique<views::WebView>(system_profile);

  // GetWebContents() creates the WebContents if it doesn't already exist.
  web_view_->GetWebContents()->SetDelegate(this);
  Observe(web_view_->web_contents());
  web_view_->LoadInitialURL(GURL(kBraveOriginStartupURL));

  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = this;

  // Widget is freed in WidgetIsZombie() after the delegate is deleted.
  auto* widget = new views::Widget();
  widget->Init(std::move(params));
  widget->CenterWindow(gfx::Size(kDialogWidth, kDialogHeight));
  widget->Show();
}

views::View* BraveOriginStartupView::GetContentsView() {
  return web_view_.get();
}

void BraveOriginStartupView::WidgetIsZombie(views::Widget* widget) {
  // Chromium checks for Widget destruction after DeleteDelegate() returns, so
  // deleting the Widget here is safe. Delete ourselves first so that web_view_
  // removes itself from the RootView before the Widget tears it down.
  delete this;
  delete widget;
}

void BraveOriginStartupView::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  // Only handle the main frame.
  if (render_frame_host != web_view_->web_contents()->GetPrimaryMainFrame()) {
    return;
  }
  SetupWebUICallbacks();
}

void BraveOriginStartupView::SetupWebUICallbacks() {
  auto* web_ui = web_view_->web_contents()->GetWebUI();
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
  if (gurl.is_valid() && delegate_) {
    delegate_->OpenExternal(gurl);
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

bool BraveOriginStartupView::HandleContextMenu(
    content::RenderFrameHost& render_frame_host,
    const content::ContextMenuParams& params) {
  // Suppress context menus: the system profile does not initialize
  // several services (ex, TemplateURLService) that are required by context
  // menu. So the default context menu crashes.
  return true;
}

void BraveOriginStartupView::WindowClosing() {
  // Clear profile pointers before shutdown destroys the profiles.
  system_profile_ = nullptr;
  default_profile_ = nullptr;

  // Release keep alives asynchronously. Destroying them synchronously during
  // WindowClosing triggers re-entrancy: on Windows via BrowserProcessImpl::
  // Unpin -> EnumThreadWindows -> CloseNow, on Linux via similar close paths.
  // Posting ensures the widget is fully destroyed before the keep alives are
  // released. The keep alive release is posted before AttemptExit so the
  // registry allows shutdown.
  auto task_runner = base::SequencedTaskRunner::GetCurrentDefault();
  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce([](std::unique_ptr<ScopedKeepAlive>,
                        std::unique_ptr<ScopedProfileKeepAlive>) {},
                     std::move(keep_alive_), std::move(profile_keep_alive_)));

  if (!validated_ && delegate_) {
    // Move delegate_ into the closure so it outlives `this` (which is deleted
    // in WidgetIsZombie right after WindowClosing returns).
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](std::unique_ptr<Delegate> delegate) { delegate->AttemptExit(); },
            std::move(delegate_)));
  }
}

bool BraveOriginStartupView::HandleKeyboardEvent(
    content::WebContents* source,
    const input::NativeWebKeyboardEvent& event) {
  return unhandled_keyboard_event_handler_.HandleKeyboardEvent(
      event, GetWidget()->GetFocusManager());
}
