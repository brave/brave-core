// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_data_handler_impl.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/speedreader/tts_player.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "ui/color/color_provider.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/browser/utils.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#endif

namespace {
class TtsPlayerDelegate : public speedreader::TtsPlayer::Delegate {
 public:
  ~TtsPlayerDelegate() override = default;

  void RequestReadingContent(
      content::WebContents* web_contents,
      base::OnceCallback<void(base::Value content)> result_cb) override {
    auto* page_distiller =
        speedreader::SpeedreaderTabHelper::GetPageDistiller(web_contents);
    if (page_distiller) {
      page_distiller->GetTextToSpeak(std::move(result_cb));
    } else {
      std::move(result_cb).Run(base::Value());
    }
  }
};

}  // namespace

SpeedreaderToolbarDataHandlerImpl::SpeedreaderToolbarDataHandlerImpl(
    Browser* browser,
    mojo::PendingReceiver<speedreader::mojom::ToolbarDataHandler> receiver,
    mojo::PendingRemote<speedreader::mojom::ToolbarEventsHandler> events)
    : browser_(browser),
      receiver_(this, std::move(receiver)),
      events_(std::move(events)),
      browser_tab_strip_tracker_(this, this) {
  if (!browser_ || !browser_->tab_strip_model() ||
      !browser_->tab_strip_model()->GetActiveWebContents()) {
    // We're initializing this handler while browser is shutting down. Do
    // nothing because we're going to die soon.
    return;
  }
  browser_tab_strip_tracker_.Init();
  active_tab_helper_ = speedreader::SpeedreaderTabHelper::FromWebContents(
      browser->tab_strip_model()->GetActiveWebContents());
  speedreader_service_observation_.Observe(GetSpeedreaderService());
  tts_player_observation_.Observe(speedreader::TtsPlayer::GetInstance());
  tab_helper_observation_.Observe(active_tab_helper_);

  speedreader::TtsPlayer::GetInstance()->set_delegate(
      std::make_unique<TtsPlayerDelegate>());

  const auto& tts_settings = GetSpeedreaderService()->GetTtsSettings();
  speedreader::TtsPlayer::GetInstance()->SetSpeed(
      static_cast<double>(tts_settings.speed) / 100.0);
  speedreader::TtsPlayer::GetInstance()->SetVoice(tts_settings.voice);
}

SpeedreaderToolbarDataHandlerImpl::~SpeedreaderToolbarDataHandlerImpl() =
    default;

void SpeedreaderToolbarDataHandlerImpl::ShowTuneBubble(bool show) {
  if (!active_tab_helper_) {
    return;
  }
  if (show) {
    active_tab_helper_->ShowSpeedreaderBubble(
        speedreader::SpeedreaderBubbleLocation::kToolbar);
  } else {
    active_tab_helper_->HideSpeedreaderBubble();
  }
}

void SpeedreaderToolbarDataHandlerImpl::GetAppearanceSettings(
    GetAppearanceSettingsCallback callback) {
  if (auto* service = GetSpeedreaderService()) {
    std::move(callback).Run(service->GetAppearanceSettings().Clone());
  } else {
    std::move(callback).Run(speedreader::mojom::AppearanceSettings::New());
  }
}

void SpeedreaderToolbarDataHandlerImpl::SetAppearanceSettings(
    speedreader::mojom::AppearanceSettingsPtr appearance_settings) {
  if (auto* service = GetSpeedreaderService()) {
    service->SetAppearanceSettings(*appearance_settings);
  }
}

void SpeedreaderToolbarDataHandlerImpl::GetTtsSettings(
    GetTtsSettingsCallback callback) {
  std::move(callback).Run(GetSpeedreaderService()->GetTtsSettings().Clone());
}

void SpeedreaderToolbarDataHandlerImpl::SetTtsSettings(
    speedreader::mojom::TtsSettingsPtr settings) {
  GetSpeedreaderService()->SetTtsSettings(*settings);
  speedreader::TtsPlayer::GetInstance()->SetVoice(settings->voice);
  speedreader::TtsPlayer::GetInstance()->SetSpeed(
      static_cast<double>(settings->speed) / 100.0);
}

void SpeedreaderToolbarDataHandlerImpl::ObserveThemeChange() {
  theme_observation_.Observe(
      ThemeServiceFactory::GetForProfile(browser_->profile()));
  native_theme_observation_.Observe(browser_->window()->GetNativeTheme());
  OnThemeChanged();
}

void SpeedreaderToolbarDataHandlerImpl::HideToolbar() {
  if (active_tab_helper_) {
    active_tab_helper_->OnShowOriginalPage();
  }
}

void SpeedreaderToolbarDataHandlerImpl::ViewOriginal() {
  if (active_tab_helper_) {
    active_tab_helper_->OnShowOriginalPage();
  }
}

void SpeedreaderToolbarDataHandlerImpl::AiChat() {
#if BUILDFLAG(ENABLE_AI_CHAT)
  if (!browser_ || !ai_chat::IsAIChatEnabled(browser_->profile()->GetPrefs()) ||
      !brave::IsRegularProfile(browser_->profile())) {
    return;
  }
  auto* side_panel = SidePanelUI::GetSidePanelUIForBrowser(browser_.get());
  if (!side_panel) {
    return;
  }

  if (auto entry = side_panel->GetCurrentEntryId();
      entry == SidePanelEntryId::kChatUI) {
    side_panel->Close();
  } else {
    side_panel->Show(SidePanelEntryId::kChatUI);
  }
#endif
}

void SpeedreaderToolbarDataHandlerImpl::GetPlaybackState(
    GetPlaybackStateCallback callback) {
  std::move(callback).Run(GetTabPlaybackState());
}

void SpeedreaderToolbarDataHandlerImpl::Rewind() {
  if (auto* tts = GetTtsController()) {
    tts->Rewind();
  }
}

void SpeedreaderToolbarDataHandlerImpl::Play() {
  if (auto* tts = GetTtsController()) {
    tts->Play();
  }
}

void SpeedreaderToolbarDataHandlerImpl::Pause() {
  if (auto* tts = GetTtsController()) {
    tts->Pause();
  }
}

void SpeedreaderToolbarDataHandlerImpl::Stop() {
  if (auto* tts = GetTtsController()) {
    tts->Stop();
  }
}

void SpeedreaderToolbarDataHandlerImpl::Forward() {
  if (auto* tts = GetTtsController()) {
    tts->Forward();
  }
}

void SpeedreaderToolbarDataHandlerImpl::OnToolbarStateChanged(
    speedreader::mojom::MainButtonType button) {
  current_button_ = button;
  if (current_button_ != speedreader::mojom::MainButtonType::TextToSpeech) {
    Pause();
  }
  if (active_tab_helper_) {
    active_tab_helper_->OnToolbarStateChanged(current_button_);
  }
}

speedreader::SpeedreaderService*
SpeedreaderToolbarDataHandlerImpl::GetSpeedreaderService() {
  DCHECK(browser_);
  return speedreader::SpeedreaderServiceFactory::GetForBrowserContext(
      browser_->profile());
}

speedreader::TtsPlayer::Controller*
SpeedreaderToolbarDataHandlerImpl::GetTtsController() {
  if (!active_tab_helper_) {
    return nullptr;
  }
  return &speedreader::TtsPlayer::GetInstance()->GetControllerFor(
      active_tab_helper_->web_contents());
}

speedreader::mojom::PlaybackState
SpeedreaderToolbarDataHandlerImpl::GetTabPlaybackState() {
  auto* tts = GetTtsController();
  if (tts && tts->IsPlaying()) {
    if (tts->IsPlayingRequestedWebContents()) {
      return speedreader::mojom::PlaybackState::kPlayingThisPage;
    } else {
      return speedreader::mojom::PlaybackState::kPlayingAnotherPage;
    }
  }
  return speedreader::mojom::PlaybackState::kStopped;
}

void SpeedreaderToolbarDataHandlerImpl::OnAppearanceSettingsChanged(
    const speedreader::mojom::AppearanceSettings& appearance_settings) {
  events_->OnAppearanceSettingsChanged(appearance_settings.Clone());
}

void SpeedreaderToolbarDataHandlerImpl::OnTtsSettingsChanged(
    const speedreader::mojom::TtsSettings& tts_settings) {
  events_->OnTtsSettingsChanged(tts_settings.Clone());
}

void SpeedreaderToolbarDataHandlerImpl::OnReadingStart(
    content::WebContents* web_contents) {
  events_->SetPlaybackState(
      speedreader::mojom::PlaybackState::kPlayingThisPage);
}

void SpeedreaderToolbarDataHandlerImpl::OnReadingStop(
    content::WebContents* web_contents) {
  events_->SetPlaybackState(speedreader::mojom::PlaybackState::kStopped);
}

void SpeedreaderToolbarDataHandlerImpl::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    tab_helper_observation_.Reset();
    active_tab_helper_ = nullptr;

    if (selection.new_contents) {
      active_tab_helper_ = speedreader::SpeedreaderTabHelper::FromWebContents(
          selection.new_contents);
      active_tab_helper_->OnToolbarStateChanged(current_button_);
      tab_helper_observation_.Observe(active_tab_helper_);
      events_->SetPlaybackState(GetTabPlaybackState());
    }
  }
}

bool SpeedreaderToolbarDataHandlerImpl::ShouldTrackBrowser(Browser* browser) {
  return browser_ == browser;
}

void SpeedreaderToolbarDataHandlerImpl::OnThemeChanged() {
  const auto* color_provider = browser_->window()->GetColorProvider();
  if (!color_provider) {
    return;
  }

  auto colors = speedreader::mojom::ToolbarColors::New();
  colors->background =
      color_provider->GetColor(kColorSpeedreaderToolbarBackground);
  colors->foreground =
      color_provider->GetColor(kColorSpeedreaderToolbarForeground);
  colors->border = color_provider->GetColor(kColorSpeedreaderToolbarBorder);
  if (BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser_)) {
    // The border is rendered in HTML. Hide the border by giving it the same
    // color as the background. When this feature flag is removed, consider
    // removing the border in HTML.
    colors->border =
        color_provider->GetColor(kColorSpeedreaderToolbarBackground);
  }
  colors->button_hover =
      color_provider->GetColor(kColorSpeedreaderToolbarButtonHover);
  colors->button_active =
      color_provider->GetColor(kColorSpeedreaderToolbarButtonActive);
  colors->button_active_text =
      color_provider->GetColor(kColorSpeedreaderToolbarButtonActiveText);
  colors->button_border =
      color_provider->GetColor(kColorSpeedreaderToolbarButtonBorder);
  events_->OnBrowserThemeChanged(std::move(colors));
}

void SpeedreaderToolbarDataHandlerImpl::OnNativeThemeUpdated(
    ui::NativeTheme* observed_theme) {
  // There are two types of theme update. a) The observed theme change. e.g.
  // switch between light/dark mode. b) A different theme is enabled. e.g.
  // switch between GTK and classic theme on Linux. Reset observer in case b).
  ui::NativeTheme* current_theme = browser_->window()->GetNativeTheme();
  if (observed_theme != current_theme) {
    native_theme_observation_.Reset();
    native_theme_observation_.Observe(current_theme);
  }
  OnThemeChanged();
}

void SpeedreaderToolbarDataHandlerImpl::OnTuneBubbleClosed() {
  events_->OnTuneBubbleClosed();
}

void SpeedreaderToolbarDataHandlerImpl::OnContentsReady() {
  if (active_tab_helper_) {
    active_tab_helper_->OnToolbarStateChanged(current_button_);
  }
}
