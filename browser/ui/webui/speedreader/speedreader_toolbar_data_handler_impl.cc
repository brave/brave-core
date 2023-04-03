// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/speedreader/speedreader_toolbar_data_handler_impl.h"

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
    mojo::PendingReceiver<speedreader::mojom::ToolbarDataHandler> receiver,
    mojo::PendingRemote<speedreader::mojom::ToolbarEventsHandler> events,
    Browser* browser)
    : receiver_(this, std::move(receiver)),
      events_(std::move(events)),
      browser_(browser) {
  speedreader_service_observation_.Observe(GetSpeedreaderService());
  tts_player_observation_.Observe(speedreader::TtsPlayer::GetInstance());
  speedreader::TtsPlayer::GetInstance()->set_delegate(
      std::make_unique<TtsPlayerDelegate>());
}

SpeedreaderToolbarDataHandlerImpl::~SpeedreaderToolbarDataHandlerImpl() =
    default;

void SpeedreaderToolbarDataHandlerImpl::GetSiteSettings(
    GetSiteSettingsCallback callback) {
  std::move(callback).Run(GetSpeedreaderTabHelper()->GetSiteSettings());
}

void SpeedreaderToolbarDataHandlerImpl::SetSiteSettings(
    speedreader::mojom::SiteSettingsPtr settings) {
  GetSpeedreaderTabHelper()->SetSiteSettings(*settings);
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

void SpeedreaderToolbarDataHandlerImpl::ViewOriginal() {
  GetSpeedreaderTabHelper()->OnShowOriginalPage();
}

void SpeedreaderToolbarDataHandlerImpl::Rewind() {
  GetTtsController().Rewind();
}

void SpeedreaderToolbarDataHandlerImpl::Play() {
  GetTtsController().Play();
}

void SpeedreaderToolbarDataHandlerImpl::Pause() {
  GetTtsController().Pause();
}

void SpeedreaderToolbarDataHandlerImpl::Stop() {
  GetTtsController().Stop();
}

void SpeedreaderToolbarDataHandlerImpl::Forward() {
  GetTtsController().Forward();
}

speedreader::SpeedreaderTabHelper*
SpeedreaderToolbarDataHandlerImpl::GetSpeedreaderTabHelper() {
  DCHECK(browser_);
  auto* tab_helper = speedreader::SpeedreaderTabHelper::FromWebContents(
      browser_->tab_strip_model()->GetActiveWebContents());
  DCHECK(tab_helper);
  return tab_helper;
}

speedreader::SpeedreaderService*
SpeedreaderToolbarDataHandlerImpl::GetSpeedreaderService() {
  DCHECK(browser_);
  return speedreader::SpeedreaderServiceFactory::GetForProfile(
      browser_->profile());
}

speedreader::TtsPlayer::Controller&
SpeedreaderToolbarDataHandlerImpl::GetTtsController() {
  return speedreader::TtsPlayer::GetInstance()->GetControllerFor(
      GetSpeedreaderTabHelper()->web_contents());
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
  events_->OnReadingStateChanged(speedreader::mojom::ReadingState::kPlaying);
}

void SpeedreaderToolbarDataHandlerImpl::OnReadingPause(
    content::WebContents* web_contents) {
  events_->OnReadingStateChanged(speedreader::mojom::ReadingState::kPaused);
}

void SpeedreaderToolbarDataHandlerImpl::OnReadingStop(
    content::WebContents* web_contents) {
  events_->OnReadingStateChanged(speedreader::mojom::ReadingState::kStopped);
}

void SpeedreaderToolbarDataHandlerImpl::OnReadingProgress(
    content::WebContents* web_contents,
    const std::string& element_id,
    int char_index,
    int length) {}