/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/text_embedder.h"

#include "base/files/file_path.h"
#include "base/location.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/leo_local_models_updater.h"
#include "brave/components/constants/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class TextEmbedderUnitTest : public testing::Test {
 public:
  TextEmbedderUnitTest() = default;
  ~TextEmbedderUnitTest() override = default;

  void SetUp() override {
    base::FilePath test_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    model_dir_ =
        test_dir.AppendASCII("leo").AppendASCII("leo-local-models-updater");
    embedder_ = TextEmbedder::Create(
        base::FilePath(model_dir_.AppendASCII(kUniversalQAModelName)));
    ASSERT_TRUE(embedder_);
    base::RunLoop run_loop;
    embedder_->Initialize(
        base::BindLambdaForTesting([&run_loop](bool initialized) {
          ASSERT_TRUE(initialized);
          run_loop.Quit();
        }));
    run_loop.Run();
    ASSERT_TRUE(embedder_->IsInitialized());
  }

  std::vector<std::string> SplitSegments(const std::string& text) {
    std::vector<std::string> segments;
    base::RunLoop run_loop;
    embedder_->GetEmbedderTaskRunner()->PostTask(
        FROM_HERE, base::BindLambdaForTesting([&]() {
          segments = embedder_->SplitSegments(text);
          run_loop.Quit();
        }));
    run_loop.Run();
    return segments;
  }

  absl::Status EmbedSegments(ai_chat::TextEmbedder* embedder) {
    absl::Status status;
    base::RunLoop run_loop;
    embedder->GetEmbedderTaskRunner()->PostTask(
        FROM_HERE, base::BindLambdaForTesting([&]() {
          status = embedder->EmbedSegments();
          run_loop.Quit();
        }));
    run_loop.Run();
    return status;
  }

  base::expected<std::string, std::string> RefineTopKSimilarity(
      std::vector<std::pair<size_t, double>> ranked_sentences,
      uint32_t context_limit) {
    base::expected<std::string, std::string> result;
    base::RunLoop run_loop;
    embedder_->GetEmbedderTaskRunner()->PostTask(
        FROM_HERE, base::BindLambdaForTesting([&]() {
          result = embedder_->RefineTopKSimilarity(std::move(ranked_sentences),
                                                   context_limit);
          run_loop.Quit();
        }));
    run_loop.Run();
    return result;
  }

  base::expected<std::string, std::string>
  GetTopSimilarityWithPromptTilContextLimit(ai_chat::TextEmbedder* embedder,
                                            const std::string& prompt,
                                            const std::string& text,
                                            uint32_t context_limit) {
    base::expected<std::string, std::string> result;
    base::RunLoop run_loop;
    embedder->GetTopSimilarityWithPromptTilContextLimit(
        prompt, text, context_limit,
        base::BindLambdaForTesting(
            [&run_loop, &result](base::expected<std::string, std::string> r) {
              result = r;
              run_loop.Quit();
            }));
    run_loop.Run();
    return result;
  }

  size_t text_hash() const { return embedder_->text_hash_; }

  size_t embeddings_size() const { return embedder_->embeddings_.size(); }

  void SetSegments(ai_chat::TextEmbedder* embedder,
                   const std::vector<std::string>& segments) {
    embedder->segments_ = segments;
  }

  void SetSegmentSizeLimitForTesting(size_t limit) {
    TextEmbedder::SetSegmentSizeLimitForTesting(limit);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::FilePath model_dir_;
  std::unique_ptr<ai_chat::TextEmbedder> embedder_;
};

TEST_F(TextEmbedderUnitTest, Create) {
  EXPECT_FALSE(TextEmbedder::Create(base::FilePath()));
  // Invalid model path is tested in TextEmbedderUnitTest.Initialize.
  EXPECT_TRUE(TextEmbedder::Create(
      base::FilePath(model_dir_.AppendASCII("model.tflite"))));
  EXPECT_TRUE(TextEmbedder::Create(
      base::FilePath(model_dir_.AppendASCII(kUniversalQAModelName))));
}

TEST_F(TextEmbedderUnitTest, Initialize) {
  auto embedder = TextEmbedder::Create(model_dir_.AppendASCII("model.tflite"));
  ASSERT_TRUE(embedder);
  base::RunLoop run_loop;
  embedder->Initialize(
      base::BindLambdaForTesting([&run_loop](bool initialized) {
        EXPECT_FALSE(initialized);
        run_loop.Quit();
      }));
  run_loop.Run();
  EXPECT_FALSE(embedder->IsInitialized());

  auto result = GetTopSimilarityWithPromptTilContextLimit(
      embedder.get(), "prompt", "text text", 5);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "TextEmbedder is not initialized.");

  SetSegments(embedder.get(), {"This is the way.", "I have spoken.",
                               "Wherever I Go, He Goes."});
  auto status = EmbedSegments(embedder.get());
  EXPECT_TRUE(absl::IsFailedPrecondition(status));
  EXPECT_EQ(status.ToString(),
            "FAILED_PRECONDITION: TextEmbedder is not initialized.");
}

TEST_F(TextEmbedderUnitTest, SplitSegments) {
  struct {
    std::string input;
    std::vector<std::string> expected;
  } test_cases[] = {{"", {}},
                    {"Hello", {"Hello"}},
                    {"Hello.", {"Hello."}},
                    {"Hello. World!", {"Hello", "World!"}},
                    {"Hello, World. Brave!", {"Hello, World", "Brave!"}},
                    {"Hello.World. This. is. the way.",
                     {"Hello.World", "This", "is", "the way."}},
                    {"IP address. 127.0.0.1", {"IP address", "127.0.0.1"}}};
  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.input);
    EXPECT_EQ(test_case.expected, SplitSegments(test_case.input));
  }

  constexpr char kSegmentedText[] =
      "A. B. C. D. E. F. G. H. I. J. K. L. M. N. "
      "O. P. Q. R. S. T. U. V. W. X. Y. Z";
  struct {
    size_t segments_size_limit;
    std::vector<std::string> expected;
  } segments_size_test_cases[] = {
      {1, {"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z"}},
      {2, {"A B C D E F G H I J K L M", "N O P Q R S T U V W X Y Z"}},
      {3, {"A B C D E F G H", "I J K L M N O P", "Q R S T U V W X", "Y Z"}},
      {5,
       {"A B C D E", "F G H I J", "K L M N O", "P Q R S T", "U V W X Y", "Z"}},
      {7,
       {"A B C", "D E F", "G H I", "J K L", "M N O", "P Q R", "S T U", "V W X",
        "Y Z"}},
      {11,
       {"A B", "C D", "E F", "G H", "I J", "K L", "M N", "O P", "Q R", "S T",
        "U V", "W X", "Y Z"}},
      {13,
       {"A B", "C D", "E F", "G H", "I J", "K L", "M N", "O P", "Q R", "S T",
        "U V", "W X", "Y Z"}},
      {17, {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"}},
      {19, {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"}},
      {23, {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"}},
  };
  for (const auto& test_case : segments_size_test_cases) {
    SCOPED_TRACE(test_case.segments_size_limit);
    SetSegmentSizeLimitForTesting(test_case.segments_size_limit);
    EXPECT_EQ(test_case.expected, SplitSegments(kSegmentedText));
  }
}

TEST_F(TextEmbedderUnitTest, EmbedSegments) {
  // Test empty segments.
  auto status = EmbedSegments(embedder_.get());
  EXPECT_FALSE(status.ok());
  EXPECT_TRUE(absl::IsFailedPrecondition(status));
  EXPECT_EQ(status.ToString(), "FAILED_PRECONDITION: No segments to embed.");
  EXPECT_EQ(embeddings_size(), 0u);

  SetSegments(embedder_.get(), {"This is the way.", "I have spoken.",
                                "Wherever I Go, He Goes."});
  EXPECT_TRUE(EmbedSegments(embedder_.get()).ok());
  EXPECT_EQ(embeddings_size(), 3u);

  SetSegments(embedder_.get(), {"This is the way.", "I have spoken."});
  EXPECT_TRUE(EmbedSegments(embedder_.get()).ok());
  EXPECT_EQ(embeddings_size(), 2u);
}

TEST_F(TextEmbedderUnitTest, RefineTopKSimilarity) {
  struct {
    std::vector<std::string> segments;
    std::vector<std::pair<size_t, double>> ranked_sentences;
    uint32_t context_limit;
    bool has_value;
    std::string expected;
  } test_cases[] = {
      {{},
       {{1, 2}, {3, 4}},
       10,
       false,
       "Segments and ranked sentences size mismatch."},
      {{"A"},
       {{1, 2}, {3, 4}},
       10,
       false,
       "Segments and ranked sentences size mismatch."},
      {{"A", "B"},
       {{1, 2}, {3, 4}},
       10,
       false,
       "Invalid ranked sentence index."},
      {{"A", "B", "C"},
       {{1, 2}, {3, 4}},
       10,
       false,
       "Segments and ranked sentences size mismatch."},
      {{"A", "B", "C", "D"},
       {{0, 20}, {1, 40}, {2, 60}, {3, 80}},
       10,
       true,
       "A. B. C. D"},
      {{"A", "B", "C", "D"},
       {{0, 20}, {1, 40}, {2, 60}, {3, 80}},
       3,
       true,
       "B. C. D"},
      {{"A", "B", "C", "D"},
       {{0, 20}, {3, 80}, {1, 40}, {2, 60}},
       3,
       true,
       "B. C. D"},
      {{"A", "B", "C", "D"},
       {{0, 20}, {3, 80}, {1, 40}, {2, 60}},
       2,
       true,
       "C. D"},
      {{"A", "B", "C", "D"},
       {{0, 20}, {3, 80}, {1, 40}, {2, 60}},
       1,
       true,
       "D"},
      {{"A", "B", "C", "D"}, {{0, 20}, {3, 80}, {1, 40}, {2, 60}}, 0, true, ""},
  };
  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i) {
    SCOPED_TRACE("Test case index: " + base::NumberToString(i));
    SetSegments(embedder_.get(), test_cases[i].segments);
    auto result = RefineTopKSimilarity(test_cases[i].ranked_sentences,
                                       test_cases[i].context_limit);
    if (test_cases[i].has_value) {
      EXPECT_TRUE(result.has_value());
      EXPECT_EQ(result.value(), test_cases[i].expected);
    } else {
      EXPECT_FALSE(result.has_value());
      EXPECT_EQ(result.error(), test_cases[i].expected);
    }
  }
}

TEST_F(TextEmbedderUnitTest, GetTopSimilarityWithPromptTilContextLimit) {
  ASSERT_EQ(text_hash(), 0u);
  constexpr char kText[] = "lion. moose. banana. alien";
  uint32_t kTextLength = static_cast<uint32_t>(strlen(kText));
  struct {
    std::string prompt;
    std::string text;
    uint32_t context_limit;
    std::string expected;
    bool has_value;
  } no_embedding_cases[] = {
      {"fruit", kText, 100, kText, true},
      {"fruit", kText, kTextLength, kText, true},
      {"", kText, kTextLength - 1, "Empty text or prompt.", false},
      {"", kText, kTextLength + 1, "Empty text or prompt.", false},
      {"cool", "", kTextLength - 1, "Empty text or prompt.", false},
      {"pool", "", kTextLength + 1, "Empty text or prompt.", false},
  };
  for (const auto& test_case : no_embedding_cases) {
    SCOPED_TRACE(
        "Prompt :" + test_case.prompt + " Text :" + test_case.text +
        " Context Limit :" + base::NumberToString(test_case.context_limit));
    auto result = GetTopSimilarityWithPromptTilContextLimit(
        embedder_.get(), test_case.prompt, test_case.text,
        test_case.context_limit);
    EXPECT_EQ(text_hash(), 0u);
    ASSERT_EQ(result.has_value(), test_case.has_value);
    if (test_case.has_value) {
      EXPECT_EQ(result.value(), test_case.expected);
    } else {
      EXPECT_EQ(result.error(), test_case.expected);
    }
  }

  // Embedding result is different on Android
#if !BUILDFLAG(IS_ANDROID)
  auto result = GetTopSimilarityWithPromptTilContextLimit(embedder_.get(),
                                                          "fruit", kText, 10);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "banana");
  EXPECT_NE(text_hash(), 0u);
  auto current_text_hash = text_hash();

  result = GetTopSimilarityWithPromptTilContextLimit(embedder_.get(), "canada",
                                                     kText, 10);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "moose");
  EXPECT_EQ(text_hash(), current_text_hash);

  result = GetTopSimilarityWithPromptTilContextLimit(
      embedder_.get(), "water", "relief. tissue. cross. liar. river. attract.",
      12);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "tissue. river");
  EXPECT_NE(text_hash(), current_text_hash);
#endif
}

}  // namespace ai_chat
