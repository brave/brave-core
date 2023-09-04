// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_DATA_HANDLER_IMPL_H_
#define BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_DATA_HANDLER_IMPL_H_

#include <string>

#include "base/scoped_observation.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/components/speedreader/common/speedreader_toolbar.mojom.h"
#include "brave/components/speedreader/speedreader_service.h"
#include "brave/components/speedreader/tts_player.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_observer.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/browser_tab_strip_tracker_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_observer.h"

class Browser;

class SpeedreaderToolbarDataHandlerImpl
    : public speedreader::mojom::ToolbarDataHandler,
      public speedreader::SpeedreaderService::Observer,
      public speedreader::TtsPlayer::Observer,
      public TabStripModelObserver,
      public BrowserTabStripTrackerDelegate,
      public ThemeServiceObserver,
      public ui::NativeThemeObserver,
      public speedreader::SpeedreaderTabHelper::Observer {
 public:
  SpeedreaderToolbarDataHandlerImpl(
      Browser* browser,
      mojo::PendingReceiver<speedreader::mojom::ToolbarDataHandler> receiver,
      mojo::PendingRemote<speedreader::mojom::ToolbarEventsHandler> events);
  SpeedreaderToolbarDataHandlerImpl(const SpeedreaderToolbarDataHandlerImpl&) =
      delete;
  SpeedreaderToolbarDataHandlerImpl& operator=(
      const SpeedreaderToolbarDataHandlerImpl&) = delete;

  ~SpeedreaderToolbarDataHandlerImpl() override;

  // speedreader::mojom::ToolbarDataHandler overrides
  void ShowTuneBubble(bool show) override;
  void GetAppearanceSettings(GetAppearanceSettingsCallback callback) override;
  void SetAppearanceSettings(
      speedreader::mojom::AppearanceSettingsPtr appearance_settings) override;

  void GetTtsSettings(GetTtsSettingsCallback callback) override;
  void SetTtsSettings(speedreader::mojom::TtsSettingsPtr settings) override;

  void ObserveThemeChange() override;

  void HideToolbar() override;

  void ViewOriginal() override;

  void AiChat() override;

  void GetPlaybackState(GetPlaybackStateCallback callback) override;
  void Rewind() override;
  void Play() override;
  void Pause() override;
  void Stop() override;
  void Forward() override;

 private:
  speedreader::SpeedreaderService* GetSpeedreaderService();
  speedreader::TtsPlayer::Controller* GetTtsController();
  speedreader::mojom::PlaybackState GetTabPlaybackState();

  // speedreader::SpeedreaderService::Observer:
  void OnAppearanceSettingsChanged(const speedreader::mojom::AppearanceSettings&
                                       appearance_settings) override;
  void OnTtsSettingsChanged(
      const speedreader::mojom::TtsSettings& tts_settings) override;

  // speedreader::TtsPlayer::Observer:
  void OnReadingStart(content::WebContents* web_contents) override;
  void OnReadingStop(content::WebContents* web_contents) override;
  void OnReadingProgress(content::WebContents* web_contents,
                         const std::string& element_id,
                         int char_index,
                         int length) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // BrowserTabStripTrackerDelegate:
  bool ShouldTrackBrowser(Browser* browser) override;

  // ThemeServiceObserver:.
  void OnThemeChanged() override;

  // ui::NativeThemeObserver:
  void OnNativeThemeUpdated(ui::NativeTheme* observed_theme) override;

  // speedreader::SpeedreaderTabHelper::Observer:
  void OnTuneBubbleClosed() override;

  raw_ptr<Browser> browser_ = nullptr;
  mojo::Receiver<speedreader::mojom::ToolbarDataHandler> receiver_;
  mojo::Remote<speedreader::mojom::ToolbarEventsHandler> events_;

  base::ScopedObservation<speedreader::SpeedreaderService,
                          speedreader::SpeedreaderService::Observer>
      speedreader_service_observation_{this};
  base::ScopedObservation<speedreader::TtsPlayer,
                          speedreader::TtsPlayer::Observer>
      tts_player_observation_{this};

  raw_ptr<speedreader::SpeedreaderTabHelper> active_tab_helper_ = nullptr;
  BrowserTabStripTracker browser_tab_strip_tracker_;

  base::ScopedObservation<ThemeService, ThemeServiceObserver>
      theme_observation_{this};
  base::ScopedObservation<ui::NativeTheme, ui::NativeThemeObserver>
      native_theme_observation_{this};
  base::ScopedObservation<speedreader::SpeedreaderTabHelper,
                          speedreader::SpeedreaderTabHelper::Observer>
      tab_helper_observation_{this};

  base::WeakPtrFactory<SpeedreaderToolbarDataHandlerImpl> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SPEEDREADER_SPEEDREADER_TOOLBAR_DATA_HANDLER_IMPL_H_
