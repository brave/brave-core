// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SPEEDREADER_TTS_PLAYER_H_
#define BRAVE_COMPONENTS_SPEEDREADER_TTS_PLAYER_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "content/public/browser/tts_utterance.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

namespace speedreader {

class TtsPlayer {
 public:
  struct Delegate {
    virtual ~Delegate() = default;

    virtual void RequestReadingContent(
        content::WebContents* web_contents,
        base::OnceCallback<void(bool success, std::string content)>
            result_cb) = 0;
  };

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnReadingStart(content::WebContents* web_contents) {}
    virtual void OnReadingPause(content::WebContents* web_contents) {}
    virtual void OnReadingStop(content::WebContents* web_contents) {}
    virtual void OnReadingProgress(content::WebContents* web_contents,
                                   const std::string& element_id,
                                   int char_index,
                                   int length) {}

   protected:
    ~Observer() override = default;
  };

  class Controller : public content::WebContentsObserver,
                     public content::UtteranceEventDelegate {
   public:
    bool IsPlaying() const;

    void Play();
    void Pause();
    void Resume();
    void Stop();

    void Forward();
    void Rewind();

   private:
    explicit Controller(TtsPlayer* owner);
    ~Controller() override;

    friend class TtsPlayer;

    bool IsPlayingRequestedWebContents() const;
    void SetRequestWebContents(content::WebContents* web_contents);

    void Resume(bool recreate_utterance);

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
                        bool success,
                        std::string content);

    TtsPlayer* owner_ = nullptr;

    raw_ptr<content::WebContents> playing_web_contents_ = nullptr;
    raw_ptr<content::WebContents> request_web_contents_ = nullptr;

    int reading_start_position_ = 0;
    int reading_position_ = 0;
    std::string reading_content_;

    double current_speed_ = 1.0;
    std::string current_voice_;
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
