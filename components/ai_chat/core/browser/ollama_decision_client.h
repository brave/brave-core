// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_DECISION_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_DECISION_CLIENT_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "url/gurl.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace ai_chat {

// Asks a decision model (for example Clef-flash) through the Ollama System
// One API: POST {model, state, questions} and get the probability of each
// answer. Each sentence goes in its own request, because the model answers
// less well when one request holds many sentences.
class OllamaDecisionClient : public MemoryDecisionClient {
 public:
  // Returns nullptr when the feature params have no endpoint or no model.
  static std::unique_ptr<OllamaDecisionClient> CreateFromFeatures(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  OllamaDecisionClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      GURL endpoint,
      std::string model);
  OllamaDecisionClient(const OllamaDecisionClient&) = delete;
  OllamaDecisionClient& operator=(const OllamaDecisionClient&) = delete;
  ~OllamaDecisionClient() override;

  // MemoryDecisionClient:
  void AskGate(const std::string& turn_text, GateCallback callback) override;
  void AskSentenceDecisions(std::vector<std::string> sentences,
                            SentenceDecisionsCallback callback) override;
  void AskRelations(const std::string& new_memory,
                    std::vector<std::string> old_memories,
                    RelationsCallback callback) override;
  void AskRelevance(const std::string& message,
                    const std::string& previous_message,
                    std::vector<std::string> memories,
                    RelevanceCallback callback) override;

 private:
  // std::nullopt when the request fails or the response is not valid.
  using AnswersCallback =
      base::OnceCallback<void(std::optional<base::DictValue> answers)>;

  using AllAnswersCallback = base::OnceCallback<void(
      std::optional<std::vector<base::DictValue>> answers)>;

  // Chat time questions go before Dreaming questions.
  enum class Priority { kDreaming, kChatTime };

  // |questions| is JSON text, in the order that the model must see. A Dreaming
  // question waits while a chat time question is active.
  void Ask(base::Value state,
           std::string questions,
           AnswersCallback callback,
           Priority priority = Priority::kDreaming);
  void Send(base::Value state,
            std::string questions,
            AnswersCallback callback,
            Priority priority);
  // Sends one request for each state at the same time. Gives the answers in
  // the order of |states|, or std::nullopt when one request fails.
  void AskEach(std::vector<base::Value> states,
               const std::string& questions,
               AllAnswersCallback callback);
  void OnResponse(std::unique_ptr<network::SimpleURLLoader> loader,
                  AnswersCallback callback,
                  Priority priority,
                  std::optional<std::string> body);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const GURL endpoint_;
  const std::string model_;

  // The chat time questions that wait for an answer, and the Dreaming
  // questions that wait for them.
  int chat_time_requests_ = 0;
  std::vector<base::OnceClosure> deferred_requests_;

  base::WeakPtrFactory<OllamaDecisionClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_OLLAMA_DECISION_CLIENT_H_
