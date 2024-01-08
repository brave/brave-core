// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/tts_player.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/tts_controller.h"
#include "content/public/browser/tts_utterance.h"
#include "content/public/browser/web_contents.h"

namespace {
constexpr const char kParagraphsKey[] = "paragraphs";
}

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

void TtsPlayer::Controller::SetRequestWebContents(
    content::WebContents* web_contents) {
  request_web_contents_ = web_contents->GetWeakPtr();
}

bool TtsPlayer::Controller::IsPlaying() const {
  auto* tts = content::TtsController::GetInstance();
  return tts->IsSpeaking();
}

bool TtsPlayer::Controller::IsPlayingRequestedWebContents(
    std::optional<int> paragraph_index) const {
  if (paragraph_index.has_value() && paragraph_index != paragraph_index_) {
    return false;
  }
  return playing_web_contents_.get() == request_web_contents_.get();
}

void TtsPlayer::Controller::Play(std::optional<int> paragraph_index) {
  CHECK(request_web_contents_);
  if (IsPlayingRequestedWebContents()) {
    Observe(playing_web_contents_.get());
    if (paragraph_index.has_value() && paragraph_index != paragraph_index_) {
      paragraph_index_ = paragraph_index.value();
      reading_start_position_ = 0;
      reading_position_ = 0;
    }
    Resume(true);
  } else {
    Stop();
    TtsPlayer::GetInstance()->delegate_->RequestReadingContent(
        request_web_contents_.get(),
        base::BindOnce(&Controller::OnContentReady, base::Unretained(this),
                       request_web_contents_.get(),
                       std::move(paragraph_index)));
  }
}

void TtsPlayer::Controller::Pause() {
  if (IsPlayingRequestedWebContents()) {
    auto* tts = content::TtsController::GetInstance();
    reading_start_position_ =
        std::min(static_cast<int>(GetParagraphToRead().size()),
                 reading_start_position_ + reading_position_);
    reading_position_ = 0;
    tts->Stop();
  } else {
    Stop();
  }
}

void TtsPlayer::Controller::Resume() {
  const bool start_new = !IsPlayingRequestedWebContents();
  Resume(start_new);
}

void TtsPlayer::Controller::Stop() {
  auto* tts = content::TtsController::GetInstance();
  tts->Stop();

  paragraph_index_ = -1;
  reading_position_ = 0;
  reading_start_position_ = 0;
  for (auto& o : owner_->observers_) {
    o.OnReadingProgress(playing_web_contents_.get(), paragraph_index_, 0, 0);
  }
  playing_web_contents_ = nullptr;

  Observe(nullptr);
}

void TtsPlayer::Controller::Forward() {
  if (!HasNextParagraph()) {
    return;
  }
  ++paragraph_index_;
  reading_start_position_ = 0;
  reading_position_ = 0;
  if (IsPlaying()) {
    Resume(true);
  } else {
    for (auto& o : owner_->observers_) {
      o.OnReadingProgress(request_web_contents_.get(), paragraph_index_, 0, 0);
    }
  }
}

void TtsPlayer::Controller::Rewind() {
  if (paragraph_index_ > 0) {
    --paragraph_index_;
  }
  reading_start_position_ = 0;
  reading_position_ = 0;
  if (IsPlaying()) {
    Resume(true);
  } else {
    for (auto& o : owner_->observers_) {
      o.OnReadingProgress(request_web_contents_.get(), paragraph_index_, 0, 0);
    }
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

    utterance->SetText(base::UTF16ToUTF8(
        GetParagraphToRead().substr(reading_start_position_)));
    utterance->SetShouldClearQueue(true);
    utterance->SetEventDelegate(this);
    utterance->SetVoiceName(current_voice_);
    const auto& params = utterance->GetContinuousParameters();
    utterance->SetContinuousParameters(current_speed_, params.pitch,
                                       params.volume);
    tts->SpeakOrEnqueue(std::move(utterance));
  }
}

bool TtsPlayer::Controller::HasNextParagraph() {
  if (!reading_content_.is_dict()) {
    return false;
  }
  const auto* content = reading_content_.GetDict().FindList(kParagraphsKey);
  if (!content) {
    return false;
  }
  return paragraph_index_ + 1 < static_cast<int>(content->size());
}

std::u16string TtsPlayer::Controller::GetParagraphToRead() {
  if (!reading_content_.is_dict()) {
    return {};
  }
  const auto* content = reading_content_.GetDict().FindList(kParagraphsKey);
  if (!content) {
    return {};
  }
  if (0 <= paragraph_index_ &&
      paragraph_index_ < static_cast<int>(content->size())) {
    return base::UTF8ToUTF16((*content)[paragraph_index_].GetString());
  }
  return {};
}

void TtsPlayer::Controller::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!handle->IsInPrimaryMainFrame() || handle->IsSameDocument()) {
    return;
  }
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
        o.OnReadingProgress(playing_web_contents_.get(), paragraph_index_,
                            reading_start_position_ + char_index, length);
      }
      break;
    case content::TtsEventType::TTS_EVENT_ERROR:
    case content::TtsEventType::TTS_EVENT_INTERRUPTED:
    case content::TtsEventType::TTS_EVENT_CANCELLED:
    case content::TtsEventType::TTS_EVENT_PAUSE:
      if (!continue_next_paragraph_) {
        for (auto& o : owner_->observers_) {
          o.OnReadingStop(playing_web_contents_.get());
        }
      }
      break;
    case content::TtsEventType::TTS_EVENT_END:
      reading_position_ = 0;
      reading_start_position_ = 0;

      if (HasNextParagraph()) {
        ++paragraph_index_;
        continue_next_paragraph_ = true;
        Resume(true);
      } else {
        paragraph_index_ = -1;
        continue_next_paragraph_ = false;
        for (auto& o : owner_->observers_) {
          o.OnReadingProgress(playing_web_contents_.get(), paragraph_index_,
                              char_index, length);
          o.OnReadingStop(playing_web_contents_.get());
        }
      }
      break;
    case content::TtsEventType::TTS_EVENT_RESUME:
    case content::TtsEventType::TTS_EVENT_START:
      if (!continue_next_paragraph_) {
        for (auto& o : owner_->observers_) {
          o.OnReadingStart(playing_web_contents_.get());
        }
      }
      continue_next_paragraph_ = false;
      break;
    case content::TTS_EVENT_SENTENCE:
    case content::TTS_EVENT_MARKER:
      break;
  }
}

void TtsPlayer::Controller::OnContentReady(content::WebContents* web_contents,
                                           std::optional<int> paragraph_index,
                                           base::Value content) {
  if (!content.is_dict() || web_contents != request_web_contents_.get()) {
    return;
  }
  playing_web_contents_ = web_contents->GetWeakPtr();

  Observe(playing_web_contents_.get());

  paragraph_index_ = paragraph_index.value_or(0);
  reading_content_ = std::move(content);
  reading_position_ = 0;
  reading_start_position_ = 0;
  Resume(true);
}

}  // namespace speedreader
