/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/speedreader/speedreader_tab_helper.h"

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/speedreader/page_distiller.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/ui/page_action/brave_page_action_icon_type.h"
#include "brave/browser/ui/speedreader/speedreader_bubble_view.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/speedreader_extended_info_handler.h"
#include "brave/components/speedreader/speedreader_rewriter_service.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/speedreader_util.h"
#include "brave/components/speedreader/tts_player.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
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
    std::initializer_list<std::pair<std::string_view, int>> resources) {
  base::Value::Dict sr_data;
  sr_data.Set("ttsEnabled", kSpeedreaderTTS.Get());
  for (const auto& r : resources) {
    sr_data.Set(r.first, brave_l10n::GetLocalizedResourceUTF16String(r.second));
  }

  return base::StrCat(
      {u"speedreaderData = ",
       base::UTF8ToUTF16(base::WriteJson(sr_data).value_or("{}"))});
}

SpeedreaderTabHelper::SpeedreaderTabHelper(
    content::WebContents* web_contents,
    SpeedreaderRewriterService* rewriter_service)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<SpeedreaderTabHelper>(*web_contents),
      PageDistiller(web_contents),
      rewriter_service_(rewriter_service) {
  dom_distiller::AddObserver(web_contents, this);
  speedreader_service_observation_.Observe(GetSpeedreaderService());
  tts_player_observation_.Observe(speedreader::TtsPlayer::GetInstance());
}

SpeedreaderTabHelper::~SpeedreaderTabHelper() {
  DCHECK(!speedreader_bubble_);
  DCHECK(!speedreader_service_observation_.IsObserving());
}

// static
void SpeedreaderTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
  if (!base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature)) {
    return;
  }

  auto* rewriter_service =
      g_brave_browser_process->speedreader_rewriter_service();
  if (!rewriter_service) {
    CHECK_IS_TEST();
    return;
  }

  SpeedreaderTabHelper::CreateForWebContents(contents, rewriter_service);
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
  if (DistillStates::IsViewOriginal(distill_state_)) {
    const auto& vo = absl::get<DistillStates::ViewOriginal>(distill_state_);
    if (!vo.was_auto_distilled ||
        !GetSpeedreaderService()->IsEnabledForSite(web_contents())) {
      GetDistilledHTML(
          base::BindOnce(&SpeedreaderTabHelper::OnGetDocumentSource,
                         weak_factory_.GetWeakPtr()));
    } else {
      TransitStateTo(DistillStates::Distilling(
          DistillStates::Distilling::Reason::kAutomatic));
    }
  } else if (DistillStates::IsDistilled(distill_state_)) {
    OnShowOriginalPage();
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
  if (DistillStates::IsDistilled(state)) {
    if (handle->IsServedFromBackForwardCache() ||
        DistillStates::IsDistilledAutomatically(state)) {
      distill_state_ = state;
      return true;
    }
  }
  SpeedreaderExtendedInfoHandler::ClearPersistedData(entry);

  return false;
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
  Browser* browser = chrome::FindBrowserWithTab(contents);
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

void SpeedreaderTabHelper::OnShowOriginalPage() {
  if (!DistillStates::IsDistilled(distill_state_)) {
    return;
  }
  TransitStateTo(DistillStates::ViewOriginal());
}

void SpeedreaderTabHelper::OnTtsPlayPause(int paragraph_index) {
  auto& tts_controller =
      speedreader::TtsPlayer::GetInstance()->GetControllerFor(web_contents());
  if (tts_controller.IsPlaying() &&
      tts_controller.IsPlayingRequestedWebContents(paragraph_index)) {
    tts_controller.Pause();
  } else {
    tts_controller.Play(paragraph_index);
  }
}

void SpeedreaderTabHelper::OnToolbarStateChanged(mojom::MainButtonType button) {
  switch (button) {
    case mojom::MainButtonType::None:
      SetDocumentAttribute("data-toolbar-button", "");
      break;
    case mojom::MainButtonType::Tune:
      SetDocumentAttribute("data-toolbar-button", "tune");
      break;
    case mojom::MainButtonType::Appearance:
      SetDocumentAttribute("data-toolbar-button", "appearance");
      break;
    case mojom::MainButtonType::TextToSpeech:
      SetDocumentAttribute("data-toolbar-button", "tts");
      break;
    case mojom::MainButtonType::AI:
      SetDocumentAttribute("data-toolbar-button", "ai");
      break;
  }
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
    content::NavigationHandle* navigation_handle,
    bool finish_navigation) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      navigation_handle->IsSameDocument() ||
      MaybeUpdateCachedState(navigation_handle)) {
    UpdateUI();
    return;
  }

  if (finish_navigation) {
    if (navigation_handle->IsErrorPage() ||
        web_contents()->GetPrimaryMainFrame()->IsErrorDocument()) {
      TransitStateTo(
          DistillStates::ViewOriginal(
              DistillStates::ViewOriginal::Reason::kNotDistillable, false),
          true);
    }
    return;
  }

  if (DistillStates::IsDistilling(distill_state_)) {
    // State will be determined in OnDistillComplete.
    return;
  }
  if (DistillStates::IsDistillReverting(distill_state_)) {
    TransitStateTo(DistillStates::ViewOriginal(), true);
    return;
  }

  auto* nav_entry = navigation_handle->GetNavigationEntry();
  const bool url_looks_readable =
      nav_entry && nav_entry->GetVirtualURL().SchemeIsHTTPOrHTTPS() &&
      rewriter_service_->URLLooksReadable(navigation_handle->GetURL());

  const bool enabled_for_site =
      GetSpeedreaderService()->IsEnabledForSite(navigation_handle->GetURL());

  const auto reason =
      url_looks_readable ? DistillStates::ViewOriginal::Reason::kNone
                         : DistillStates::ViewOriginal::Reason::kNotDistillable;
  TransitStateTo(DistillStates::DistillReverting(reason, false), true);
  TransitStateTo(DistillStates::ViewOriginal(), true);

  if (enabled_for_site) {
    // Check if url is pointed to the homepage, basically these pages aren't
    // readable. We've got the same check in speedreader::IsURLLooksReadable
    const bool homepage = !navigation_handle->GetURL().has_path() ||
                          navigation_handle->GetURL().path_piece() == "/";

    // Enable speedreader if the user explicitly enabled speedreader on the
    // site.
    const bool explicit_enabled_for_size =
        !homepage && kSpeedreaderExplicitPref.Get() &&
        GetSpeedreaderService()->GetEnabledForSiteSetting(
            navigation_handle->GetURL());
    if (url_looks_readable || explicit_enabled_for_size) {
      // Speedreader enabled for this page.
      TransitStateTo(DistillStates::Distilling(
                         DistillStates::Distilling::Reason::kAutomatic),
                     true);
    }
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
  if (const auto* browser = chrome::FindBrowserWithTab(web_contents())) {
    if (!DistillStates::IsDistilled(PageDistillState())) {
      static_cast<BraveBrowserWindow*>(browser->window())
          ->HideReaderModeToolbar();
    } else {
      static_cast<BraveBrowserWindow*>(browser->window())
          ->ShowReaderModeToolbar();
    }

    browser->window()->UpdatePageActionIcon(
        brave::kSpeedreaderPageActionIconType);
  }
#endif
}

void SpeedreaderTabHelper::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame()) {
    return;
  }

  blink::web_pref::WebPreferences prefs =
      web_contents()->GetOrCreateWebPreferences();
  prefs.page_in_reader_mode = DistillStates::IsDistilled(PageDistillState());
  web_contents()->SetWebPreferences(prefs);
}

void SpeedreaderTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessNavigation(navigation_handle);
}

void SpeedreaderTabHelper::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessNavigation(navigation_handle);
}

void SpeedreaderTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  ProcessNavigation(navigation_handle, true);
}

void SpeedreaderTabHelper::DidStopLoading() {
  auto* entry = web_contents()->GetController().GetLastCommittedEntry();
  if (entry) {
    SpeedreaderExtendedInfoHandler::PersistMode(entry, distill_state_);
  }
}

void SpeedreaderTabHelper::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host->IsInPrimaryMainFrame() ||
      !DistillStates::IsDistilled(distill_state_)) {
    return;
  }
  UpdateUI();

  static base::NoDestructor<std::u16string> kSpeedreaderData(GetSpeedreaderData(
      {{"showOriginalLinkText", IDS_READER_MODE_SHOW_ORIGINAL_PAGE_LINK},
       {"minutesText", IDS_READER_MODE_MINUTES_TEXT},
#if defined(IDS_READER_MODE_TEXT_TO_SPEECH_PLAY_PAUSE)
       {"playButtonTitle", IDS_READER_MODE_TEXT_TO_SPEECH_PLAY_PAUSE}
#endif
      }));

  static base::NoDestructor<std::u16string> kJsScript(base::UTF8ToUTF16(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_SPEEDREADER_JS_DESKTOP)));

  static base::NoDestructor<std::u16string> kLoadScript(*kSpeedreaderData +
                                                        *kJsScript);

  render_frame_host->ExecuteJavaScriptInIsolatedWorld(
      *kLoadScript, base::DoNothing(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);

  for (auto& o : observers_) {
    o.OnContentsReady();
  }
}

void SpeedreaderTabHelper::OnVisibilityChanged(content::Visibility visibility) {
  is_visible_ = visibility != content::Visibility::HIDDEN;
  UpdateUI();
}

void SpeedreaderTabHelper::WebContentsDestroyed() {
  speedreader_service_observation_.Reset();
  tts_player_observation_.Reset();
  dom_distiller::RemoveObserver(web_contents(), this);
  SetWebContents(nullptr);
  HideSpeedreaderBubble();
}

bool SpeedreaderTabHelper::IsPageDistillationAllowed() {
  return DistillStates::IsDistilling(distill_state_) ||
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
  Transit(distill_state_, DistillStates::Distilled(result));
}

void SpeedreaderTabHelper::OnDistilledDocumentSent() {
  UpdateUI();

#if BUILDFLAG(IS_ANDROID)
  // Attempt to reset page scale after a successful distillation.
  // This is done by mocking a pinch gesture on Android,
  // see chrome/android/java/src/org/chromium/chrome/browser/ZoomController.java
  // and ui/android/event_forwarder.cc
  if (DistillStates::IsDistilled(distill_state_)) {
    ui::ViewAndroid* view = web_contents()->GetNativeView();
    int64_t time_ms = base::TimeTicks::Now().ToUptimeMillis();
    SendGestureEvent(view, ui::GESTURE_EVENT_TYPE_PINCH_BEGIN, time_ms, 0.f);
    SendGestureEvent(view, ui::GESTURE_EVENT_TYPE_PINCH_BY, time_ms, -1.f);
    SendGestureEvent(view, ui::GESTURE_EVENT_TYPE_PINCH_END, time_ms, 0.f);
  }
#endif
}

void SpeedreaderTabHelper::OnReadingStart(content::WebContents* web_contents) {
  if (!speedreader::DistillStates::IsDistilled(distill_state_)) {
    return;
  }

  static constexpr char16_t kReading[] =
      uR"js( speedreaderUtils.setTtsReadingState($1) )js";

  const auto script = base::ReplaceStringPlaceholders(
      kReading, (web_contents == this->web_contents()) ? u"true" : u"false",
      nullptr);

  this->web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      script, base::DoNothing(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void SpeedreaderTabHelper::OnReadingStop(content::WebContents* web_contents) {
  OnReadingStart(nullptr);
}

void SpeedreaderTabHelper::OnReadingProgress(content::WebContents* web_contents,
                                             int paragraph_index,
                                             int char_index,
                                             int length) {
  if (!speedreader::DistillStates::IsDistilled(distill_state_) ||
      web_contents != this->web_contents()) {
    return;
  }
  static constexpr char16_t kHighlight[] =
      uR"js( speedreaderUtils.highlightText($1, $2, $3) )js";

  const auto script = base::ReplaceStringPlaceholders(
      kHighlight,
      {base::NumberToString16(paragraph_index),
       base::NumberToString16(char_index), base::NumberToString16(length)},
      nullptr);

  this->web_contents()->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      script, base::DoNothing(), ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

void SpeedreaderTabHelper::OnSiteEnableSettingChanged(
    content::WebContents* site,
    bool enabled_on_site) {
  if (site != web_contents()) {
    return;
  }

  if (enabled_on_site) {
    TransitStateTo(DistillStates::Distilling(
        DistillStates::Distilled::Reason::kAutomatic));
  } else {
    TransitStateTo(DistillStates::ViewOriginal());
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
  SetDocumentAttribute("data-column-width",
                       speedreader_service->GetColumnWidth());
}

void SpeedreaderTabHelper::OnResult(
    const dom_distiller::DistillabilityResult& result) {
  if (DistillStates::IsNotDistillable(distill_state_) &&
      result.is_distillable) {
    TransitStateTo(DistillStates::DistillReverting(
        DistillStates::DistillReverting::Reason::kNone, false));
    TransitStateTo(DistillStates::ViewOriginal());
  }
}

void SpeedreaderTabHelper::SetDocumentAttribute(const std::string& attribute,
                                                const std::string& value) {
  static constexpr char16_t kSetAttribute[] =
      uR"js(
    (function() {
      const attribute = '$1'
      const value = '$2'
      if (value == '') {
        document?.documentElement?.removeAttribute(attribute)
      } else {
        document?.documentElement?.setAttribute(attribute, value)
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
  if (!success || html.empty()) {
    // TODO(boocmp): Show error dialog [Distillation failed on this page].
    TransitStateTo(DistillStates::DistillReverting(
        DistillStates::DistillReverting::Reason::kError, false));
    TransitStateTo(DistillStates::ViewOriginal());
    return;
  }

  single_show_content_ = std::move(html);
  TransitStateTo(
      DistillStates::Distilling(DistillStates::Distilling::Reason::kManual));
}

SpeedreaderService* SpeedreaderTabHelper::GetSpeedreaderService() {
  return SpeedreaderServiceFactory::GetForBrowserContext(
      web_contents()->GetBrowserContext());
}

void SpeedreaderTabHelper::TransitStateTo(const DistillState& desired_state,
                                          bool no_reload) {
  if (Transit(distill_state_, desired_state) && !no_reload) {
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
