/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/speech/speech_synthesis.h"

#include "base/compiler_specific.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"

#define OnSetVoiceList OnSetVoiceList_ChromiumImpl
#include "src/third_party/blink/renderer/modules/speech/speech_synthesis.cc"
#undef OnSetVoiceList

namespace blink {

void SpeechSynthesis::OnSetVoiceList(
    Vector<mojom::blink::SpeechSynthesisVoicePtr> mojom_voices) {
  voice_list_.clear();
  BraveFarblingLevel farbling_level = brave::GetBraveFarblingLevelFor(
      GetExecutionContext(),
      ContentSettingsType::BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS,
      BraveFarblingLevel::OFF);
  if (farbling_level == BraveFarblingLevel::OFF) {
    // farbling off -> call upstream function
    OnSetVoiceList_ChromiumImpl(std::move(mojom_voices));
    return;
  }
  if (farbling_level == BraveFarblingLevel::MAXIMUM) {
    // maximum farbling -> return empty voice list
    VoicesDidChange();
    return;
  }
  mojom::blink::SpeechSynthesisVoicePtr fake_voice;
  for (auto& mojom_voice : mojom_voices) {
    if (!fake_voice && mojom_voice->is_default) {
      // balanced farbling -> return real voices + one fake voice
      if (ExecutionContext* context = GetExecutionContext()) {
        fake_voice = mojom_voice.Clone();
        fake_voice->is_default = false;
        brave::FarblingPRNG prng = brave::BraveSessionCache::From(*context)
                                       .MakePseudoRandomGenerator();
        const char* kFakeNames[] = {
            "Hubert", "Vernon", "Rudolph",   "Clayton",    "Irving",
            "Wilson", "Alva",   "Harley",    "Beauregard", "Cleveland",
            "Cecil",  "Reuben", "Sylvester", "Jasper"};
        const int kFakeNamesCount = std::size(kFakeNames);
        fake_voice->name =
            UNSAFE_TODO(WTF::String(kFakeNames[prng() % kFakeNamesCount]));
      }
    }
    voice_list_.push_back(
        MakeGarbageCollected<SpeechSynthesisVoice>(std::move(mojom_voice)));
  }
  if (fake_voice) {
    voice_list_.push_back(
        MakeGarbageCollected<SpeechSynthesisVoice>(std::move(fake_voice)));
  }
  VoicesDidChange();
}

}  // namespace blink
