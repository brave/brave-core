// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/history_search_tool.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/ai_chat/ai_chat_conversation_ui_browsertest_base.h"
#include "brave/browser/ai_chat/browser_tool_provider.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/history/core/browser/url_row.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/history_embeddings/core/vector_database.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Sequence;

namespace {

// Canned visit time for the row returned by the fake. The expected
// last_visit_time field in test JSON is derived from this same value via
// TimeFormatAsIso8601 so the two halves can't drift.
const base::Time& CannedVisitTime() {
  static const base::Time time = base::Time::FromTimeT(1700000000);
  return time;
}

// Minimal fake that records args and fires a canned result. The real
// HistoryEmbeddingsService also implements this interface, so the tool sees
// nothing unusual.
class FakeHistoryEmbeddingsSearch
    : public history_embeddings::HistoryEmbeddingsSearch {
 public:
  history_embeddings::SearchResult Search(
      history_embeddings::SearchResult* previous_search_result,
      std::string query,
      std::optional<base::Time> time_range_start,
      size_t count,
      bool skip_answering,
      history_embeddings::SearchResultCallback callback) override {
    last_query_ = query;
    last_time_range_start_ = time_range_start;
    last_count_ = count;
    last_skip_answering_ = skip_answering;

    history_embeddings::SearchResult result;
    result.query = query;
    history_embeddings::ScoredUrl scored_url(
        /*url_id=*/1, /*visit_id=*/1, base::Time::Now(),
        /*score=*/0.9f, /*word_match_score=*/0.0f);
    history_embeddings::ScoredUrlRow row(scored_url);
    row.row.set_url(GURL(canned_url_));
    row.row.set_title(u"Canned title");
    row.row.set_last_visit(CannedVisitTime());
    result.scored_url_rows.push_back(std::move(row));

    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(callback, std::move(result)));
    return history_embeddings::SearchResult();
  }

  std::string last_query_;
  std::optional<base::Time> last_time_range_start_;
  size_t last_count_ = 0;
  bool last_skip_answering_ = false;
  std::string canned_url_ = "https://example.com/canned";
};

}  // namespace

class HistorySearchToolBrowserTest
    : public AIChatConversationUIBrowserTestBase {
 public:
  HistorySearchToolBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        history_embeddings::kHistoryEmbeddings);
  }

  void SetUpOnMainThread() override {
    // IsHistoryEmbeddingsEnabledForProfile gates the tool on both the
    // brave-history-embeddings feature flag and a Brave-owned pref. Enable
    // the pref so BrowserToolProvider::CreateTools actually constructs the
    // HistorySearchTool we're exercising here.
    browser()->profile()->GetPrefs()->SetBoolean(
        local_ai::prefs::kBraveHistoryEmbeddingsEnabled, true);
    AIChatConversationUIBrowserTestBase::SetUpOnMainThread();
  }

 protected:
  // Reroutes the conversation's production HistorySearchTool to a fake
  // search service so that real HistoryEmbeddingsService calls are avoided.
  void InjectFakeSearch(FakeHistoryEmbeddingsSearch* fake) {
    auto* provider = static_cast<BrowserToolProvider*>(
        conversation_handler_->GetFirstToolProviderForTesting());
    ASSERT_TRUE(provider);
    auto* tool = provider->GetHistorySearchToolForTesting();
    ASSERT_TRUE(tool);
    tool->SetSearchForTesting(fake);
  }

  // Waits for the tool's permission challenge to land in conversation
  // history, then approves it so the tool can execute.
  void WaitForAndApprovePermissionChallenge(
      const std::string& tool_id,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    ASSERT_TRUE(base::test::RunUntil([this]() {
      for (const auto& turn : conversation_handler_->GetConversationHistory()) {
        if (!turn->events.has_value()) {
          continue;
        }
        for (const auto& event : *turn->events) {
          if (event->is_tool_use_event() &&
              event->get_tool_use_event()->permission_challenge) {
            return true;
          }
        }
      }
      return false;
    }));
    conversation_handler_->ProcessPermissionChallenge(tool_id, true);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

// End-to-end: the engine emits a tool_use event referencing
// semantic_history_search; ConversationHandler dispatches to the real
// HistorySearchTool; the tool forwards the parsed arguments to the search
// service and returns the JSON-serialized result back into the conversation.
IN_PROC_BROWSER_TEST_F(HistorySearchToolBrowserTest,
                       ToolUseDispatchesSearchAndReturnsResult) {
  CreateConversationWithMockEngine();

  FakeHistoryEmbeddingsSearch fake;
  InjectFakeSearch(&fake);

  // After our tool returns, ConversationHandler will issue a follow-up
  // GenerateAssistantResponse to feed the tool result back to the model.
  // We don't care about that second call -- absorb it.
  Sequence seq;
  auto generate_future = SetupMockGenerateAssistantResponse(&seq);
  EXPECT_CALL(*mock_engine_, GenerateAssistantResponse(_, _, _, _, _, _, _, _))
      .Times(AnyNumber())
      .InSequence(seq);
  conversation_handler_->SubmitHumanConversationEntry(
      "Find the article I read about kittens", std::nullopt);
  auto callbacks = generate_future->Take();

  base::DictValue args;
  args.Set("query", "kittens article");
  args.Set("count", 3);
  args.Set("time_range_start_days_ago", 7);
  args.Set("include_all_passages", false);
  std::string args_json = *base::WriteJson(args);

  callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          mojom::kSemanticHistorySearchToolName, "tool_id_1", args_json,
          std::nullopt, std::nullopt, nullptr, false)),
      std::nullopt));
  std::move(callbacks.completed_callback)
      .Run(base::ok(
          EngineConsumer::GenerationResultData(nullptr, std::nullopt)));

  // The tool requires user permission before sending results to the LLM.
  WaitForAndApprovePermissionChallenge("tool_id_1");

  // Wait for ConversationHandler to dispatch to our tool, which forwards
  // the query to the fake search service.
  ASSERT_TRUE(
      base::test::RunUntil([&fake]() { return !fake.last_query_.empty(); }));

  EXPECT_EQ(fake.last_query_, "kittens article");
  EXPECT_EQ(fake.last_count_, 3u);
  EXPECT_TRUE(fake.last_skip_answering_)
      << "The tool must request the embeddings-only path -- answering is "
         "handled by the Leo model itself.";
  ASSERT_TRUE(fake.last_time_range_start_.has_value());
  EXPECT_LT(base::Time::Now() - *fake.last_time_range_start_,
            base::Days(7) + base::Minutes(5));

  // The tool's output is plumbed back into the conversation as the
  // tool_use_event's `output`. Wait for it to land before inspecting.
  auto find_completed_tool_use = [this]() -> const mojom::ToolUseEvent* {
    for (const auto& turn : conversation_handler_->GetConversationHistory()) {
      if (!turn->events.has_value()) {
        continue;
      }
      for (const auto& event : *turn->events) {
        if (!event->is_tool_use_event()) {
          continue;
        }
        const auto& tool_use = event->get_tool_use_event();
        if (tool_use->tool_name == mojom::kSemanticHistorySearchToolName &&
            tool_use->output.has_value() && !tool_use->output->empty()) {
          return tool_use.get();
        }
      }
    }
    return nullptr;
  };
  ASSERT_TRUE(base::test::RunUntil(
      [&find_completed_tool_use]() { return find_completed_tool_use(); }));

  const mojom::ToolUseEvent* tool_use = find_completed_tool_use();
  ASSERT_TRUE(tool_use);
  const auto& blocks = *tool_use->output;
  ASSERT_FALSE(blocks.empty());
  ASSERT_TRUE(blocks[0]->is_text_content_block());
  EXPECT_THAT(blocks[0]->get_text_content_block()->text,
              base::test::IsJson(
                  base::StrCat({R"({
        "query": "kittens article",
        "results": [{
          "title": "Canned title",
          "url": "https://example.com/canned",
          "last_visit_time": ")",
                                base::TimeFormatAsIso8601(CannedVisitTime()),
                                R"("
        }]
      })"})));
}

// When the engine sends malformed JSON args, the tool surfaces an error
// rather than crashing, and the conversation continues.
IN_PROC_BROWSER_TEST_F(HistorySearchToolBrowserTest,
                       MalformedArgsProducesErrorOutput) {
  CreateConversationWithMockEngine();

  FakeHistoryEmbeddingsSearch fake;
  InjectFakeSearch(&fake);

  // After our tool returns, ConversationHandler will issue a follow-up
  // GenerateAssistantResponse to feed the tool result back to the model.
  // We don't care about that second call -- absorb it.
  Sequence seq;
  auto generate_future = SetupMockGenerateAssistantResponse(&seq);
  EXPECT_CALL(*mock_engine_, GenerateAssistantResponse(_, _, _, _, _, _, _, _))
      .Times(AnyNumber())
      .InSequence(seq);
  conversation_handler_->SubmitHumanConversationEntry("anything", std::nullopt);
  auto callbacks = generate_future->Take();

  callbacks.data_callback.Run(EngineConsumer::GenerationResultData(
      mojom::ConversationEntryEvent::NewToolUseEvent(mojom::ToolUseEvent::New(
          mojom::kSemanticHistorySearchToolName, "tool_id_err", "{ not json",
          std::nullopt, std::nullopt, nullptr, false)),
      std::nullopt));
  std::move(callbacks.completed_callback)
      .Run(base::ok(
          EngineConsumer::GenerationResultData(nullptr, std::nullopt)));

  // The permission challenge fires before UseTool runs; approve it so the
  // tool actually executes and surfaces its parse error.
  WaitForAndApprovePermissionChallenge("tool_id_err");

  // Wait for ConversationHandler to dispatch and our tool to surface its
  // error output back into the conversation.
  ASSERT_TRUE(base::test::RunUntil([this]() {
    for (const auto& turn : conversation_handler_->GetConversationHistory()) {
      if (!turn->events.has_value()) {
        continue;
      }
      for (const auto& event : *turn->events) {
        if (event->is_tool_use_event() &&
            event->get_tool_use_event()->tool_name ==
                mojom::kSemanticHistorySearchToolName &&
            event->get_tool_use_event()->output.has_value() &&
            !event->get_tool_use_event()->output->empty()) {
          return true;
        }
      }
    }
    return false;
  }));

  EXPECT_TRUE(fake.last_query_.empty())
      << "Search service must not be called with unparseable args";
}

}  // namespace ai_chat
