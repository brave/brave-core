// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/tts_player.h"

#include <algorithm>
#include <utility>

#include "content/public/browser/tts_controller.h"
#include "content/public/browser/tts_utterance.h"
#include "content/public/browser/web_contents.h"

namespace speedreader {

TtsPlayer::TtsPlayer() = default;

TtsPlayer::~TtsPlayer() = default;

// static
TtsPlayer* TtsPlayer::GetInstance() {
  return base::Singleton<TtsPlayer>::get();
}

TtsPlayer::Controller& TtsPlayer::GetControllerFor(
    content::WebContents* web_contents) {
  controller_.SetRequestWebContents(web_contents);
  return controller_;
}

void TtsPlayer::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void TtsPlayer::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

double TtsPlayer::GetSpeed() const {
  return controller_.current_speed_;
}

const std::string& TtsPlayer::GetVoice() const {
  return controller_.current_voice_;
}

void TtsPlayer::SetSpeed(double speed) {
  if (std::abs(speed - controller_.current_speed_) > 0.05) {
    controller_.current_speed_ = speed;
    if (controller_.IsPlaying()) {
      controller_.Resume(true);
    }
  }
}

void TtsPlayer::SetVoice(const std::string& voice) {
  if (controller_.current_voice_ != voice) {
    controller_.current_voice_ = voice;
    if (controller_.IsPlaying()) {
      controller_.Resume(true);
    }
  }
}

TtsPlayer::Controller::Controller(TtsPlayer* owner) : owner_(owner) {}
TtsPlayer::Controller::~Controller() = default;

bool TtsPlayer::Controller::IsPlayingRequestedWebContents() const {
  return playing_web_contents_ == request_web_contents_;
}

void TtsPlayer::Controller::SetRequestWebContents(
    content::WebContents* web_contents) {
  request_web_contents_ = web_contents;
}

bool TtsPlayer::Controller::IsPlaying() const {
  auto* tts = content::TtsController::GetInstance();
  return tts->IsSpeaking();
}

void TtsPlayer::Controller::Play() {
  DCHECK(request_web_contents_);
  if (IsPlayingRequestedWebContents()) {
    Observe(playing_web_contents_);
    Resume(true);
  } else {
    Stop();
    TtsPlayer::GetInstance()->delegate_->RequestReadingContent(
        request_web_contents_,
        base::BindOnce(&Controller::OnContentReady, base::Unretained(this),
                       request_web_contents_));
  }
}

void TtsPlayer::Controller::Pause() {
  auto* tts = content::TtsController::GetInstance();
  tts->Pause();
}

void TtsPlayer::Controller::Resume() {
  const bool start_new = !IsPlayingRequestedWebContents();
  Resume(start_new);
}

void TtsPlayer::Controller::Stop() {
  auto* tts = content::TtsController::GetInstance();
  tts->Stop();
  reading_position_ = 0;
  reading_start_position_ = 0;
  playing_web_contents_ = nullptr;

  Observe(nullptr);
}

void TtsPlayer::Controller::Forward() {
  reading_start_position_ =
      std::min(static_cast<int>(reading_content_.size()),
               reading_start_position_ + reading_position_ + 32);
  reading_position_ = 0;
  if (IsPlaying()) {
    Resume(true);
  }
}

void TtsPlayer::Controller::Rewind() {
  reading_start_position_ =
      std::max(0, reading_start_position_ + reading_position_ - 32);
  reading_position_ = 0;
  if (IsPlaying()) {
    Resume(true);
  }
}

void TtsPlayer::Controller::Resume(bool recreate_utterance) {
  auto* tts = content::TtsController::GetInstance();
  if (!recreate_utterance) {
    tts->Resume();
  } else {
    auto utterance = content::TtsUtterance::Create();

    reading_start_position_ += reading_position_;
    reading_position_ = 0;

    utterance->SetText(reading_content_.substr(reading_start_position_));
    utterance->SetShouldClearQueue(true);
    utterance->SetEventDelegate(this);
    utterance->SetVoiceName(current_voice_);
    const auto& params = utterance->GetContinuousParameters();
    utterance->SetContinuousParameters(current_speed_, params.pitch,
                                       params.volume);
    tts->SpeakOrEnqueue(std::move(utterance));
  }
}

void TtsPlayer::Controller::DidStartNavigation(
    content::NavigationHandle* handle) {
  Stop();
}

void TtsPlayer::Controller::WebContentsDestroyed() {
  Stop();
}

void TtsPlayer::Controller::OnTtsEvent(content::TtsUtterance* utterance,
                                       content::TtsEventType event_type,
                                       int char_index,
                                       int length,
                                       const std::string& error_message) {
  switch (event_type) {
    case content::TtsEventType::TTS_EVENT_WORD:
      reading_position_ = char_index;
      for (auto& o : owner_->observers_) {
        o.OnReadingProgress(playing_web_contents_, "content", char_index,
                            length);
      }
      break;
    case content::TtsEventType::TTS_EVENT_ERROR:
    case content::TtsEventType::TTS_EVENT_INTERRUPTED:
    case content::TtsEventType::TTS_EVENT_CANCELLED:
    case content::TtsEventType::TTS_EVENT_END:
      for (auto& o : owner_->observers_) {
        o.OnReadingStop(playing_web_contents_);
      }
      if (event_type == content::TtsEventType::TTS_EVENT_END) {
        reading_position_ = 0;
        reading_start_position_ = 0;
      }
      break;
    case content::TtsEventType::TTS_EVENT_RESUME:
    case content::TtsEventType::TTS_EVENT_START:
      for (auto& o : owner_->observers_) {
        o.OnReadingStart(playing_web_contents_);
      }
      break;
    case content::TtsEventType::TTS_EVENT_PAUSE:
      for (auto& o : owner_->observers_) {
        o.OnReadingPause(playing_web_contents_);
      }
      break;
    case content::TTS_EVENT_SENTENCE:
    case content::TTS_EVENT_MARKER:
      break;
  }
}

void TtsPlayer::Controller::OnContentReady(content::WebContents* web_contents,
                                           bool success,
                                           std::string content) {
  if (!success || web_contents != request_web_contents_) {
    return;
  }
  playing_web_contents_ = web_contents;

  Observe(playing_web_contents_);

  reading_content_ = std::move(content);
  reading_position_ = 0;
  reading_start_position_ = 0;
  Resume(true);
}

}  // namespace speedreader
