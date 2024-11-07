/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_H_

#include <optional>
#include <string>

#include "content/browser/speech/speech_recognition_engine.h"

#define NetworkSpeechRecognitionEngineImpl \
  NetworkSpeechRecognitionEngineImpl_ChromiumImpl

#include "src/content/browser/speech/network_speech_recognition_engine_impl.h"  // IWYU pragma: export

#undef NetworkSpeechRecognitionEngineImpl

namespace content {

class CONTENT_EXPORT NetworkSpeechRecognitionEngineImpl
    : public NetworkSpeechRecognitionEngineImpl_ChromiumImpl {
 public:
  NetworkSpeechRecognitionEngineImpl(
      scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory);
  ~NetworkSpeechRecognitionEngineImpl() override;

  void StartRecognition() override;

 private:
  void OnStickySessionReady(std::unique_ptr<network::SimpleURLLoader> loader,
                            std::optional<std::string> response_body);

  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::WeakPtrFactory<NetworkSpeechRecognitionEngineImpl> weak_ptr_factory_{
      this};
};

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_SPEECH_NETWORK_SPEECH_RECOGNITION_ENGINE_IMPL_H_
