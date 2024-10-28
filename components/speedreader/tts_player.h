// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_TTS_PLAYER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_TTS_PLAYER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "content/public/browser/tts_utterance.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

namespace speedreader {

// Browser-wide singleton that provides Text-to-speech functionality for
// speedreader.
class TtsPlayer {
 public:
  // Delegate for getting a text content of the WebContents for playing.
  // TtsPlayer owns this in unique_ptr.
  struct Delegate {
    virtual ~Delegate() = default;

    virtual void RequestReadingContent(
        content::WebContents* web_contents,
        base::OnceCallback<void(base::Value content)> result_cb) = 0;
  };

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnReadingStart(content::WebContents* web_contents) {}
    virtual void OnReadingStop(content::WebContents* web_contents) {}
    virtual void OnReadingProgress(content::WebContents* web_contents,
                                   int tts_order,
                                   int char_index,
                                   int length) {}

   protected:
    ~Observer() override = default;
  };

  // Provides tts control fucntions for specified WebContents (provided by
  // TtsPlayer::GetControllerFor). Controller is a part of TtsPlayer and has
  // same lifetime.
  class Controller : public content::WebContentsObserver,
                     public content::UtteranceEventDelegate {
   public:
    bool IsPlaying() const;
    bool IsPlayingRequestedWebContents(
        std::optional<int> paragraph_index = std::nullopt) const;

    void Play(std::optional<int> paragraph_index = std::nullopt);
    void Pause();
    void Resume();
    void Stop();
    void Forward();
    void Rewind();

   private:
    explicit Controller(TtsPlayer* owner);
    ~Controller() override;

    friend class TtsPlayer;

    void SetRequestWebContents(content::WebContents* web_contents);

    void Resume(bool recreate_utterance);

    bool HasNextParagraph();
    std::u16string GetParagraphToRead();

    // content::WebContentsObserver:
    void DidStartNavigation(content::NavigationHandle* handle) override;
    void WebContentsDestroyed() override;

    // content::UtteranceEventDelegate:
    void OnTtsEvent(content::TtsUtterance* utterance,
                    content::TtsEventType event_type,
                    int char_index,
                    int length,
                    const std::string& error_message) override;

    void OnContentReady(content::WebContents* web_contents,
                        std::optional<int> paragraph_index,
                        base::Value content);

    raw_ptr<TtsPlayer> owner_ = nullptr;

    raw_ptr<content::WebContents, DanglingUntriaged> playing_web_contents_ =
        nullptr;
    raw_ptr<content::WebContents, DanglingUntriaged> request_web_contents_ =
        nullptr;

    int paragraph_index_ = -1;
    int reading_start_position_ = 0;
    int reading_position_ = 0;
    base::Value reading_content_;

    double current_speed_ = 1.0;
    std::string current_voice_;

    bool continue_next_paragraph_ = false;
  };

  ~TtsPlayer();

  TtsPlayer(const TtsPlayer&) = delete;
  TtsPlayer(TtsPlayer&&) = delete;
  TtsPlayer& operator=(const TtsPlayer&) = delete;

  static TtsPlayer* GetInstance();

  void set_delegate(std::unique_ptr<Delegate> delegate) {
    delegate_ = std::move(delegate);
  }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  double GetSpeed() const;
  const std::string& GetVoice() const;

  void SetSpeed(double speed);
  void SetVoice(const std::string& voice);

  Controller& GetControllerFor(content::WebContents* web_contents);

 private:
  friend struct base::DefaultSingletonTraits<TtsPlayer>;

  TtsPlayer();

  std::unique_ptr<Delegate> delegate_;
  Controller controller_{this};
  base::ObserverList<Observer> observers_;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_TTS_PLAYER_H_
