// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_data_handler_impl.h"

#include <memory>
#include <utility>

#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/components/speedreader/tts_player.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace {
class TtsPlayerDelegate : public speedreader::TtsPlayer::Delegate {
 public:
  ~TtsPlayerDelegate() override = default;

  void RequestReadingContent(
      content::WebContents* web_contents,
      base::OnceCallback<void(bool success, std::string content)> result_cb)
      override {
    auto* page_distiller =
        speedreader::SpeedreaderTabHelper::GetPageDistiller(web_contents);
    if (page_distiller) {
      page_distiller->GetDistilledText(std::move(result_cb));
    } else {
      std::move(result_cb).Run(false, {});
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
  browser_tab_strip_tracker_.Init();
  active_tab_helper_ = speedreader::SpeedreaderTabHelper::FromWebContents(
      browser->tab_strip_model()->GetActiveWebContents());
  speedreader_service_observation_.Observe(GetSpeedreaderService());
  tts_player_observation_.Observe(speedreader::TtsPlayer::GetInstance());
  speedreader::TtsPlayer::GetInstance()->set_delegate(
      std::make_unique<TtsPlayerDelegate>());
  SetTtsSettings(GetSpeedreaderService()->GetTtsSettings().Clone());
}

SpeedreaderToolbarDataHandlerImpl::~SpeedreaderToolbarDataHandlerImpl() =
    default;

void SpeedreaderToolbarDataHandlerImpl::GetSiteSettings(
    GetSiteSettingsCallback callback) {
  if (active_tab_helper_) {
    std::move(callback).Run(active_tab_helper_->GetSiteSettings());
  } else {
    std::move(callback).Run(speedreader::mojom::SiteSettings::New());
  }
}

void SpeedreaderToolbarDataHandlerImpl::SetSiteSettings(
    speedreader::mojom::SiteSettingsPtr settings) {
  if (active_tab_helper_) {
    active_tab_helper_->SetSiteSettings(*settings);
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

void SpeedreaderToolbarDataHandlerImpl::HideToolbar() {
  if (active_tab_helper_) {
    active_tab_helper_->HideReaderModeToolbar();
  }
}

void SpeedreaderToolbarDataHandlerImpl::ViewOriginal() {
  if (active_tab_helper_) {
    active_tab_helper_->OnShowOriginalPage();
  }
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

speedreader::SpeedreaderService*
SpeedreaderToolbarDataHandlerImpl::GetSpeedreaderService() {
  DCHECK(browser_);
  return speedreader::SpeedreaderServiceFactory::GetForProfile(
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

void SpeedreaderToolbarDataHandlerImpl::OnSiteSettingsChanged(
    const speedreader::mojom::SiteSettings& site_settings) {
  events_->OnSiteSettingsChanged(site_settings.Clone());
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

void SpeedreaderToolbarDataHandlerImpl::OnReadingProgress(
    content::WebContents* web_contents,
    const std::string& element_id,
    int char_index,
    int length) {}

void SpeedreaderToolbarDataHandlerImpl::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    active_tab_helper_ = nullptr;

    if (selection.new_contents) {
      active_tab_helper_ = speedreader::SpeedreaderTabHelper::FromWebContents(
          selection.new_contents);
      events_->SetPlaybackState(GetTabPlaybackState());
    }
  }
}

bool SpeedreaderToolbarDataHandlerImpl::ShouldTrackBrowser(Browser* browser) {
  return browser_ == browser;
}
