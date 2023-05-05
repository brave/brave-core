// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_DATA_HANDLER_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_DATA_HANDLER_IMPL_H_

#include <string>

#include "base/scoped_observation.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/tts_player.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class Browser;

namespace speedreader {
class SpeedreaderTabHelper;
}  // namespace speedreader

class SpeedreaderToolbarDataHandlerImpl
    : public speedreader::mojom::ToolbarDataHandler,
      public speedreader::SpeedreaderService::Observer,
      public speedreader::TtsPlayer::Observer {
 public:
  SpeedreaderToolbarDataHandlerImpl(
      mojo::PendingReceiver<speedreader::mojom::ToolbarDataHandler> receiver,
      mojo::PendingRemote<speedreader::mojom::ToolbarEventsHandler> events,
      Browser* browser);
  SpeedreaderToolbarDataHandlerImpl(const SpeedreaderToolbarDataHandlerImpl&) =
      delete;
  SpeedreaderToolbarDataHandlerImpl& operator=(
      const SpeedreaderToolbarDataHandlerImpl&) = delete;

  ~SpeedreaderToolbarDataHandlerImpl() override;

  // speedreader::mojom::ToolbarDataHandler overrides
  void GetSiteSettings(GetSiteSettingsCallback callback) override;
  void SetSiteSettings(speedreader::mojom::SiteSettingsPtr settings) override;

  void GetTtsSettings(GetTtsSettingsCallback callback) override;
  void SetTtsSettings(speedreader::mojom::TtsSettingsPtr settings) override;

  void ViewOriginal() override;

  void Rewind() override;
  void Play() override;
  void Pause() override;
  void Stop() override;
  void Forward() override;

 private:
  speedreader::SpeedreaderTabHelper* GetSpeedreaderTabHelper();
  speedreader::SpeedreaderService* GetSpeedreaderService();
  speedreader::TtsPlayer::Controller& GetTtsController();

  // speedreader::SpeedreaderService::Observer:
  void OnSiteSettingsChanged(
      const speedreader::mojom::SiteSettings& site_settings) override;
  void OnTtsSettingsChanged(
      const speedreader::mojom::TtsSettings& tts_settings) override;

  // speedreader::TtsPlayer::Observer:
  void OnReadingStart(content::WebContents* web_contents) override;
  void OnReadingPause(content::WebContents* web_contents) override;
  void OnReadingStop(content::WebContents* web_contents) override;
  void OnReadingProgress(content::WebContents* web_contents,
                         const std::string& element_id,
                         int char_index,
                         int length) override;

  mojo::Receiver<speedreader::mojom::ToolbarDataHandler> receiver_;
  mojo::Remote<speedreader::mojom::ToolbarEventsHandler> events_;

  raw_ptr<Browser> browser_ = nullptr;

  base::ScopedObservation<speedreader::SpeedreaderService,
                          speedreader::SpeedreaderService::Observer>
      speedreader_service_observation_{this};
  base::ScopedObservation<speedreader::TtsPlayer,
                          speedreader::TtsPlayer::Observer>
      tts_player_observation_{this};

  base::WeakPtrFactory<SpeedreaderToolbarDataHandlerImpl> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_DATA_HANDLER_IMPL_H_
