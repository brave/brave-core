/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include <initializer_list>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/speedreader/page_distiller.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_extended_info_handler.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/resource/resource_bundle.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_browser_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#else
#include "ui/android/view_android.h"
#include "ui/events/android/gesture_event_android.h"
#include "ui/events/android/gesture_event_type.h"
#endif

namespace speedreader {

std::u16string GetSpeedreaderData(
    std::initializer_list<std::pair<base::StringPiece, int>> resources) {
  std::u16string result = u"speedreaderData = {";
  for (const auto& r : resources) {
    auto text = brave_l10n::GetLocalizedResourceUTF16String(r.second);
    // Make sure that the text doesn't contain js injection
    base::ReplaceChars(text, u"\"", u"\\\"", &text);
    result += base::StrCat({base::UTF8ToUTF16(r.first), u": \"", text, u"\","});
  }

  return result + u"}\n\n";
}

SpeedreaderTabHelper::SpeedreaderTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SpeedreaderTabHelper>(*web_contents),
      PageDistiller(web_contents) {
  dom_distiller::AddObserver(web_contents, this);
  speedreader_service_observation_.Observe(GetSpeedreaderService());
}

SpeedreaderTabHelper::~SpeedreaderTabHelper() {
  DCHECK(!speedreader_bubble_);
  DCHECK(!speedreader_service_observation_.IsObserving());
}

// static
void SpeedreaderTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature)) {
    SpeedreaderTabHelper::CreateForWebContents(contents);
  }
}

// static
void SpeedreaderTabHelper::BindSpeedreaderHost(
    mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver,
    content::RenderFrameHost* rfh) {
  auto* sender = content::WebContents::FromRenderFrameHost(rfh);
  if (!sender) {
    return;
  }
  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(sender);
  if (!tab_helper) {
    return;
  }
  tab_helper->BindReceiver(std::move(receiver));
}

// static
PageDistiller* SpeedreaderTabHelper::GetPageDistiller(
    content::WebContents* contents) {
  auto* tab_helper = SpeedreaderTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    return nullptr;
  }
  return tab_helper;
}

void SpeedreaderTabHelper::BindReceiver(
    mojo::PendingAssociatedReceiver<mojom::SpeedreaderHost> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

base::WeakPtr<SpeedreaderTabHelper> SpeedreaderTabHelper::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void SpeedreaderTabHelper::ProcessIconClick() {
  if (absl::holds_alternative<DistillStates::ViewOriginal>(distill_state_)) {
    GetDistilledHTML(base::BindOnce(&SpeedreaderTabHelper::OnGetDocumentSource,
                                    weak_factory_.GetWeakPtr()));
  } else if (absl::holds_alternative<DistillStates::Distilled>(
                 distill_state_)) {
    TransitStateTo(DistillStates::ViewOriginal{
        .reason = DistillStates::ViewOriginal::Reason::kUserAction,
        .url = web_contents()->GetLastCommittedURL()});
  }
}

SpeedreaderBubbleView* SpeedreaderTabHelper::speedreader_bubble_view() const {
  return speedreader_bubble_;
}

bool SpeedreaderTabHelper::MaybeUpdateCachedState(
    content::NavigationHandle* handle) {
  auto* entry = handle->GetNavigationEntry();
  if (!entry || (handle->GetRestoreType() != content::RestoreType::kRestored &&
                 !handle->IsServedFromBackForwardCache())) {
    return false;
  }
  auto* speedreader_service = GetSpeedreaderService();

  const DistillState state =
      SpeedreaderExtendedInfoHandler::GetCachedMode(entry, speedreader_service);
  const bool cached = !absl::holds_alternative<DistillStates::None>(state);
  if (cached) {
    distill_state_ = state;
  } else {
    SpeedreaderExtendedInfoHandler::ClearPersistedData(entry);
  }
  return cached;
}

void SpeedreaderTabHelper::OnBubbleClosed() {
  speedreader_bubble_ = nullptr;
  UpdateUI();

  for (auto& o : observers_) {
    o.OnTuneBubbleClosed();
  }
}

void SpeedreaderTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SpeedreaderTabHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SpeedreaderTabHelper::ShowSpeedreaderBubble(
    SpeedreaderBubbleLocation location) {
#if !BUILDFLAG(IS_ANDROID)
  auto* contents = web_contents();
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);

  speedreader_bubble_ = static_cast<BraveBrowserWindow*>(browser->window())
                            ->ShowSpeedreaderBubble(this, location);
#endif
}

void SpeedreaderTabHelper::HideSpeedreaderBubble() {
  if (speedreader_bubble_) {
    speedreader_bubble_->Hide();
    speedreader_bubble_ = nullptr;
  }
}

void SpeedreaderTabHelper::HideReaderModeToolbar() {
  toolbar_hidden_ = true;
  UpdateUI();
}

void SpeedreaderTabHelper::OnShowOriginalPage() {
  TransitStateTo(DistillStates::ViewOriginal{
      .reason = DistillStates::ViewOriginal::Reason::kUserAction,
      .url = web_contents()->GetLastCommittedURL()});
}

void SpeedreaderTabHelper::ClearPersistedData() {
  if (auto* entry = web_contents()->GetController().GetLastCommittedEntry()) {
    SpeedreaderExtendedInfoHandler::ClearPersistedData(entry);
  }
}

void SpeedreaderTabHelper::ReloadContents() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL, false);
}

void SpeedreaderTabHelper::ProcessNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      navigation_handle->IsSameDocument() ||
      MaybeUpdateCachedState(navigation_handle)) {
    UpdateUI();
    return;
  }

  toolbar_hidden_ = false;

  if (DistillStates::IsTransition(distill_state_)) {
    PerformStateTransition(distill_state_);
    return;
  }

  auto* rewriter_service =
      g_brave_browser_process->speedreader_rewriter_service();
  const bool url_looks_readable =
      rewriter_service &&
      rewriter_service->URLLooksReadable(navigation_handle->GetURL());

  if (url_looks_readable &&
      GetSpeedreaderService()->IsEnabledForSite(navigation_handle->GetURL())) {
    // Speedreader enabled for this page.
    TransitStateTo(
        DistillStates::Pending{.reason =
                                   DistillStates::Pending::Reason::kAutomatic},
        true);
  } else {
    TransitStateTo(
        DistillStates::ViewOriginal{.url = navigation_handle->GetURL()}, true);
  }
}

void SpeedreaderTabHelper::UpdateUI() {
  if (DistillStates::IsDistilled(distill_state_)) {
    UpdateState(State::kDistilled);
  } else if (DistillStates::IsDistillable(distill_state_)) {
    UpdateState(State::kDistillable);
  } else {
    UpdateState(State::kNotDistillable);
  }

  if (!is_visible_) {
    return;
  }
#if !BUILDFLAG(IS_ANDROID)
  if (const auto* browser =
          chrome::FindBrowserWithWebContents(web_contents())) {
    if (toolbar_hidden_ || !DistillStates::IsDistilled(PageDistillState())) {
      static_cast<BraveBrowserWindow*>(browser->window())
          ->HideReaderModeToolbar();
    } else {
      static_cast<BraveBrowserWindow*>(browser->window())
          ->ShowReaderModeToolbar();
    }

    browser->window()->UpdatePageActionIcon(PageActionIconType::kReaderMode);
  }
#endif
}

void SpeedreaderTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessNavigation(navigation_handle);
}

void SpeedreaderTabHelper::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessNavigation(navigation_handle);
}

void SpeedreaderTabHelper::DidStopLoading() {
  auto* entry = web_contents()->GetController().GetLastCommittedEntry();
  if (entry) {
    SpeedreaderExtendedInfoHandler::PersistMode(entry, distill_state_);
  }
}

void SpeedreaderTabHelper::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host->IsInPrimaryMainFrame()) {
    return;
  }

  if (!IsPageDistillationAllowed()) {
    return;
  } else {
    UpdateUI();
  }

  static base::NoDestructor<std::u16string> kSpeedreaderData(
      GetSpeedreaderData({{"minutesText", IDS_READER_MODE_MINUTES_TEXT}}));

  static base::NoDestructor<std::u16string> kJsScript(base::UTF8ToUTF16(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_SPEEDREADER_JS_DESKTOP)));

  static base::NoDestructor<std::u16string> kLoadScript(*kSpeedreaderData +
                                                        *kJsScript);

  render_frame_host->ExecuteJavaScriptInIsolatedWorld(
      *kLoadScript, base::DoNothing(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void SpeedreaderTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  is_visible_ = visibility != content::Visibility::HIDDEN;
  UpdateUI();
}

void SpeedreaderTabHelper::WebContentsDestroyed() {
  speedreader_service_observation_.Reset();
  dom_distiller::RemoveObserver(web_contents(), this);
  SetWebContents(nullptr);
  HideSpeedreaderBubble();
}

bool SpeedreaderTabHelper::IsPageDistillationAllowed() {
  return DistillStates::IsPending(distill_state_) ||
         DistillStates::IsDistilled(distill_state_);
}

bool SpeedreaderTabHelper::IsPageContentPresent() {
  return !single_show_content_.empty();
}

std::string SpeedreaderTabHelper::TakePageContent() {
  return std::move(single_show_content_);
}

void SpeedreaderTabHelper::OnDistillComplete(DistillationResult result) {
  // Perform a state transition
  TransitStateTo(DistillStates::Distilled{.result = result});

#if BUILDFLAG(IS_ANDROID)
  // Attempt to reset page scale after a successful distillation.
  // This is done by mocking a pinch gesture on Android,
  // see chrome/android/java/src/org/chromium/chrome/browser/ZoomController.java
  // and ui/android/event_forwarder.cc
  if (PageStateIsDistilled(distill_state_)) {
    ui::ViewAndroid* view = web_contents()->GetNativeView();
    int64_t time_ms = base::TimeTicks::Now().ToUptimeMillis();
    SendGestureEvent(view, ui::GESTURE_EVENT_TYPE_PINCH_BEGIN, time_ms, 0.f);
    SendGestureEvent(view, ui::GESTURE_EVENT_TYPE_PINCH_BY, time_ms, -1.f);
    SendGestureEvent(view, ui::GESTURE_EVENT_TYPE_PINCH_END, time_ms, 0.f);
  }
#endif
}

void SpeedreaderTabHelper::OnSiteEnableSettingChanged(
    content::WebContents* site,
    bool enabled_on_site) {
  if (site != web_contents()) {
    return;
  }

  if (enabled_on_site) {
    TransitStateTo(DistillStates::Pending{
        .reason = DistillStates::Pending::Reason::kManual});
  } else {
    TransitStateTo(DistillStates::ViewOriginal{
        .reason = DistillStates::ViewOriginal::Reason::kUserAction,
        .url = web_contents()->GetLastCommittedURL()});
  }
  HideSpeedreaderBubble();
}

void SpeedreaderTabHelper::OnAllSitesEnableSettingChanged(
    bool enabled_on_all_sites) {
  if (!is_visible_ || !GetSpeedreaderService()) {
    return;
  }
  OnSiteEnableSettingChanged(
      web_contents(),
      GetSpeedreaderService()->IsEnabledForSite(web_contents()));
}

void SpeedreaderTabHelper::OnAppearanceSettingsChanged(
    const mojom::AppearanceSettings& view_settings) {
  auto* speedreader_service = GetSpeedreaderService();
  if (!speedreader_service) {
    return;
  }
  if (!speedreader::DistillStates::IsDistilled(distill_state_)) {
    return;
  }

  SetDocumentAttribute("data-theme", speedreader_service->GetThemeName());
  SetDocumentAttribute("data-font-family",
                       speedreader_service->GetFontFamilyName());
  SetDocumentAttribute("data-font-size",
                       speedreader_service->GetFontSizeName());
}

void SpeedreaderTabHelper::OnResult(
    const dom_distiller::DistillabilityResult& result) {
  if (DistillStates::IsNotDistillable(distill_state_) &&
      result.is_distillable) {
    TransitStateTo(DistillStates::ViewOriginal(
        {.url = web_contents()->GetLastCommittedURL()}));
  }
}

void SpeedreaderTabHelper::SetDocumentAttribute(const std::string& attribute,
                                                const std::string& value) {
  constexpr const char16_t kSetAttribute[] =
      uR"js(
    (function() {
      const attribute = '$1'
      const value = '$2'
      if (value == '') {
        document.documentElement.removeAttribute(attribute)
      } else {
        document.documentElement.setAttribute(attribute, value)
      }
    })();
  )js";

  const auto script = base::ReplaceStringPlaceholders(
      kSetAttribute, {base::UTF8ToUTF16(attribute), base::UTF8ToUTF16(value)},
      nullptr);

  web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      script, base::DoNothing(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void SpeedreaderTabHelper::OnGetDocumentSource(bool success, std::string html) {
  if (!success) {
    // TODO(boocmp): Show error dialog [Distillation failed on this page].
    TransitStateTo(DistillStates::ViewOriginal{
        .reason = DistillStates::ViewOriginal::Reason::kError,
        .url = web_contents()->GetLastCommittedURL()});
    return;
  }

  single_show_content_.swap(html);
  TransitStateTo(DistillStates::Pending{
      .reason = DistillStates::Pending::Reason::kManual});
}

SpeedreaderService* SpeedreaderTabHelper::GetSpeedreaderService() {
  return SpeedreaderServiceFactory::GetForBrowserContext(
      web_contents()->GetBrowserContext());
}

void SpeedreaderTabHelper::TransitStateTo(DistillState desired_state,
                                          bool no_reload) {
  if (Transit(distill_state_, std::move(desired_state)) && !no_reload) {
    ClearPersistedData();
    ReloadContents();
  }
  UpdateUI();
}

#if BUILDFLAG(IS_ANDROID)
bool SpeedreaderTabHelper::SendGestureEvent(ui::ViewAndroid* view,
                                            int type,
                                            int64_t time_ms,
                                            float scale) {
  float dip_scale = view->GetDipScale();
  auto size = view->GetSize();
  float x = size.width() / 2;
  float y = size.height() / 2;
  gfx::PointF root_location =
      ScalePoint(view->GetLocationOnScreen(x, y), 1.f / dip_scale);
  return view->OnGestureEvent(ui::GestureEventAndroid(
      type, gfx::PointF(x / dip_scale, y / dip_scale), root_location, time_ms,
      scale, 0, 0, 0, 0, /*target_viewport*/ false, /*synthetic_scroll*/ false,
      /*prevent_boosting*/ false));
}
#endif

WEB_CONTENTS_USER_DATA_KEY_IMPL(SpeedreaderTabHelper);

}  // namespace speedreader
