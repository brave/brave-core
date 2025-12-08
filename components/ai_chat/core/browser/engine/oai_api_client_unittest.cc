/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"

#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using ConversationHistory = std::vector<ai_chat::mojom::ConversationTurn>;
using ::testing::_;
using ::testing::Sequence;
using DataReceivedCallback =
    api_request_helper::APIRequestHelper::DataReceivedCallback;
using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;
using GenerationResult = ai_chat::OAIAPIClient::GenerationResult;
using api_request_helper::MockAPIRequestHelper;

namespace ai_chat {

namespace {

constexpr char kTestContent[] = "test content";
constexpr char kTestImageUrl[] = "data:image/png;base64,xyz";

// A helper method which parses a string_view and returns the JSON or a
// base::Value object if the JSON is invalid.
base::Value ParseOrStringValue(const std::string& json) {
  auto maybe_json =
      base::JSONReader::Read(json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!maybe_json.has_value()) {
    return base::Value(json);
  }
  return std::move(maybe_json.value());
}

struct LocalizedText {
  int message_id = 0;
  std::optional<std::string> format_arg;
};

struct ContentBlockSerializationTestParam {
  std::string name;
  base::RepeatingCallback<ExtendedContentBlock()> content_factory;
  std::string expected_type;
  std::optional<LocalizedText> localized_text;
  std::optional<std::string> literal_text;
};

class MockCallbacks {
 public:
  MOCK_METHOD(void, OnDataReceived, (EngineConsumer::GenerationResultData));
  MOCK_METHOD(void, OnCompleted, (GenerationResult));
};

class TestOAIAPIClient : public OAIAPIClient {
 public:
  TestOAIAPIClient() : OAIAPIClient(nullptr) {
    auto mock_helper =
        std::make_unique<testing::NiceMock<MockAPIRequestHelper>>(
            net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
            nullptr);
    SetAPIRequestHelperForTesting(std::move(mock_helper));
  }
  ~TestOAIAPIClient() override = default;

  MockAPIRequestHelper* GetMockAPIRequestHelper() {
    return static_cast<MockAPIRequestHelper*>(GetAPIRequestHelperForTesting());
  }
};

}  // namespace

class OAIAPIUnitTest : public testing::Test {
 public:
  OAIAPIUnitTest() = default;
  ~OAIAPIUnitTest() override = default;

  void SetUp() override { client_ = std::make_unique<TestOAIAPIClient>(); }

  void TearDown() override { client_.reset(); }

  std::string GetMessagesJson(std::string_view body_json) {
    auto dict = base::test::ParseJsonDict(body_json);
    base::Value::List* events = dict.FindList("messages");
    EXPECT_TRUE(events);
    std::string events_json;
    base::JSONWriter::WriteWithOptions(
        *events, base::JSONWriter::OPTIONS_PRETTY_PRINT, &events_json);
    return events_json;
  }

  std::string FormatComparableEventsJson(std::string_view formatted_json) {
    auto messages = base::test::ParseJson(formatted_json);
    std::string json;
    base::JSONWriter::WriteWithOptions(
        messages, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
    return json;
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestOAIAPIClient> client_;
};

TEST_F(OAIAPIUnitTest, PerformRequest) {
  mojom::CustomModelOptionsPtr model_options = mojom::CustomModelOptions::New(
      "test_api_key", 0, 0, 0, "test_system_prompt", GURL("https://test.com"),
      "test_model");

  std::string server_chunk =
      R"({"id":"chatcmpl-123","object":"chat.completion.chunk","created":1694268190,"model":"gpt-3.5-turbo-0125", "system_fingerprint": "fp_44709d6fcb", "choices":[{"index":0,"delta":{"role":"assistant","content":"It was played in Arlington, Texas."},"logprobs":null,"finish_reason":null}]})";
  std::string server_completion =
      R"({"id":"chatcmpl-123","object":"chat.completion","created":1677652288,"model":"gpt-3.5-turbo-0125","system_fingerprint":"fp_44709d6fcb","choices":[{"index":0,"message":{"role":"assistant","content":"\n\nCan I assist you further?"},"logprobs":null,"finish_reason":"stop"}],"usage":{"prompt_tokens":9,"completion_tokens":12,"total_tokens":21}})";

  std::string expected_chunk_response = "It was played in Arlington, Texas.";
  std::string expected_completion_response = "\n\nCan I assist you further?";
  std::string expected_conversation_body = R"([
    {"role": "user", "content": "Where was it played?"}
  ])";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  // Intercept API Request Helper call and verify the request is as expected
  GURL expected_url = model_options->endpoint;
  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&, expected_url](
                    const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        EXPECT_TRUE(url.is_valid());
        EXPECT_EQ(url, expected_url);
        EXPECT_EQ(headers.contains("Authorization"), true);
        EXPECT_EQ(method, net::HttpRequestHeaders::kPostMethod);
        EXPECT_EQ(GetMessagesJson(body),
                  FormatComparableEventsJson(expected_conversation_body));

        auto chunk = base::test::ParseJson(server_chunk);
        data_received_callback.Run(base::ok(std::move(chunk)));

        auto completed = base::test::ParseJson(server_completion);
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, std::move(completed),
                                                      {}, net::OK, GURL()));

        run_loop.Quit();
        return Ticket();
      });

  EXPECT_CALL(mock_callbacks, OnDataReceived(_))
      .WillOnce([&](EngineConsumer::GenerationResultData result) {
        ASSERT_TRUE(result.event);
        ASSERT_TRUE(result.event->is_completion_event());
        EXPECT_EQ(result.event->get_completion_event()->completion,
                  expected_chunk_response);
        EXPECT_FALSE(result.model_key.has_value());
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([&](GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion,
                  expected_completion_response);
        EXPECT_FALSE(result->model_key.has_value());
      });

  // Begin request
  auto messages = base::test::ParseJsonList(expected_conversation_body);

  client_->PerformRequest(
      *model_options, std::move(messages),
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();

  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
}

TEST_F(OAIAPIUnitTest, PerformRequest_WithStopSequences) {
  mojom::CustomModelOptionsPtr model_options = mojom::CustomModelOptions::New(
      "test_api_key", 0, 0, 0, "test_system_prompt", GURL("https://test.com"),
      "test_model");

  std::vector<std::string> stop_sequences = {"/title", "END"};
  std::string expected_conversation_body = R"([
    {"role": "user", "content": "Test message"}
  ])";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        auto dict = base::test::ParseJsonDict(body);
        base::Value::List* stop_list = dict.FindList("stop");
        EXPECT_TRUE(stop_list);
        EXPECT_EQ(stop_list->size(), 2u);
        EXPECT_EQ((*stop_list)[0].GetString(), "/title");
        EXPECT_EQ((*stop_list)[1].GetString(), "END");

        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, base::Value(), {},
                                                      net::OK, GURL()));
        run_loop.Quit();
        return Ticket();
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_)).WillOnce([](auto) {});

  auto messages = base::test::ParseJsonList(expected_conversation_body);
  client_->PerformRequest(
      *model_options, std::move(messages),
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)),
      stop_sequences);

  run_loop.Run();
}

TEST_F(OAIAPIUnitTest, PerformRequest_WithEmptyStopSequences) {
  mojom::CustomModelOptionsPtr model_options = mojom::CustomModelOptions::New(
      "test_api_key", 0, 0, 0, "test_system_prompt", GURL("https://test.com"),
      "test_model");

  std::vector<std::string> empty_stop_sequences = {};
  std::string expected_conversation_body = R"([
    {"role": "user", "content": "Test message"}
  ])";

  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();
  testing::StrictMock<MockCallbacks> mock_callbacks;
  base::RunLoop run_loop;

  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](const std::string& method, const GURL& url,
                    const std::string& body, const std::string& content_type,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback,
                    const base::flat_map<std::string, std::string>& headers,
                    const api_request_helper::APIRequestOptions& options) {
        auto dict = base::test::ParseJsonDict(body);
        base::Value* stop_field = dict.Find("stop");
        EXPECT_FALSE(stop_field);

        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(200, base::Value(), {},
                                                      net::OK, GURL()));
        run_loop.Quit();
        return Ticket();
      });

  EXPECT_CALL(mock_callbacks, OnCompleted(_)).WillOnce([](auto) {});

  auto messages = base::test::ParseJsonList(expected_conversation_body);
  client_->PerformRequest(
      *model_options, std::move(messages),
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)),
      empty_stop_sequences);

  run_loop.Run();
}

TEST_F(OAIAPIUnitTest, SerializeOAIMessages) {
  // A general test which covers the serialization of multiple messages with
  // multiple content blocks, but does not cover all possible types of content
  // block. Each block type's serialization should be tested in
  // ContentBlockSerializationTest below.
  std::vector<OAIMessage> messages;

  // First message: user with multiple block types
  OAIMessage user_msg1;
  user_msg1.role = "user";
  user_msg1.content.emplace_back(ExtendedContentBlockType::kText,
                                 TextContent{"Here's an image:"});
  ImageContent img;
  img.image_url.url = kTestImageUrl;
  img.image_url.detail = "low";
  user_msg1.content.emplace_back(ExtendedContentBlockType::kImage,
                                 std::move(img));
  user_msg1.content.emplace_back(ExtendedContentBlockType::kPageExcerpt,
                                 TextContent{"Page excerpt content"});
  messages.push_back(std::move(user_msg1));

  // Second message: assistant response
  OAIMessage assistant_msg;
  assistant_msg.role = "assistant";
  assistant_msg.content.emplace_back(ExtendedContentBlockType::kText,
                                     TextContent{"I see the image"});
  messages.push_back(std::move(assistant_msg));

  // Third message: user follow-up
  OAIMessage user_msg2;
  user_msg2.role = "user";
  user_msg2.content.emplace_back(ExtendedContentBlockType::kText,
                                 TextContent{"Can you improve this?"});
  messages.push_back(std::move(user_msg2));

  auto serialized = OAIAPIClient::SerializeOAIMessages(std::move(messages));
  ASSERT_EQ(serialized.size(), 3u);

  std::string page_excerpt = l10n_util::GetStringFUTF8(
      IDS_AI_CHAT_LLAMA2_SELECTED_TEXT_PROMPT_SEGMENT,
      base::UTF8ToUTF16(std::string("Page excerpt content")));

  // First message
  const base::Value::Dict* msg0 = serialized[0].GetIfDict();
  ASSERT_TRUE(msg0);
  std::string expected_msg1_json = absl::StrFormat(R"({
    "role": "user",
    "content": [
      {"type": "text", "text": "Here's an image:"},
      {"type": "image_url", "image_url": {"detail": "low", "url": "%s"}},
      {"type": "text", "text": "%s"}
    ]
  })",
                                                   kTestImageUrl, page_excerpt);
  base::Value::Dict expected_msg1 =
      base::test::ParseJsonDict(expected_msg1_json);
  EXPECT_EQ(*msg0, expected_msg1);

  // Second message
  const base::Value::Dict* msg1 = serialized[1].GetIfDict();
  ASSERT_TRUE(msg1);
  base::Value::Dict expected_msg2 = base::test::ParseJsonDict(R"({
    "role": "assistant",
    "content": [
      {"type": "text", "text": "I see the image"}
    ]
  })");
  EXPECT_EQ(*msg1, expected_msg2);

  // Third message
  const base::Value::Dict* msg2 = serialized[2].GetIfDict();
  ASSERT_TRUE(msg2);
  base::Value::Dict expected_msg3 = base::test::ParseJsonDict(R"({
    "role": "user",
    "content": [
      {"type": "text", "text": "Can you improve this?"}
    ]
  })");
  EXPECT_EQ(*msg2, expected_msg3);
}

TEST_F(OAIAPIUnitTest, SerializeOAIMessages_SkipsMismatchedContentTypes) {
  std::vector<OAIMessage> messages;

  // Create a message with multiple content blocks:
  // 1. A text type with ImageContent (mismatched - should be skipped)
  // 2. A text type with TextContent (valid - should be included)
  // 3. An image type with TextContent (mismatched - should be skipped)
  // 4. A change_tone type with ImageContent (mismatched - should be skipped)
  OAIMessage user_message;
  user_message.role = "user";

  // Content block with kText type but ImageContent data
  // (should be skipped)
  ImageUrl image_url1;
  image_url1.url = kTestImageUrl;
  user_message.content.push_back(ExtendedContentBlock(
      ExtendedContentBlockType::kText, ImageContent{std::move(image_url1)}));

  // Content block with kText type and TextContent data
  // (should be included)
  user_message.content.push_back(ExtendedContentBlock(
      ExtendedContentBlockType::kText, TextContent{kTestContent}));

  // Content block with kImage type but TextContent data
  // (should be skipped)
  user_message.content.push_back(ExtendedContentBlock(
      ExtendedContentBlockType::kImage, TextContent{"This is not an image"}));

  // Content block with kChangeTone type but TextContent data
  // (should be skipped)
  user_message.content.push_back(
      ExtendedContentBlock(ExtendedContentBlockType::kChangeTone,
                           TextContent{"This is not a tone"}));

  messages.push_back(std::move(user_message));

  // Serialize messages
  auto serialized = OAIAPIClient::SerializeOAIMessages(std::move(messages));
  ASSERT_EQ(serialized.size(), 1u);

  // Expected output: only the valid text block
  std::string expected_message_json = absl::StrFormat(R"({
    "role": "user",
    "content": [
      {"type": "text", "text": "%s"}
    ]
  })",
                                                      kTestContent);
  base::Value::Dict expected_message =
      base::test::ParseJsonDict(expected_message_json);

  const base::Value::Dict* msg = serialized[0].GetIfDict();
  ASSERT_TRUE(msg);
  EXPECT_EQ(*msg, expected_message);
}

// Tests to cover serialization of all content block types.
class ContentBlockSerializationTest
    : public OAIAPIUnitTest,
      public testing::WithParamInterface<ContentBlockSerializationTestParam> {};

TEST_P(ContentBlockSerializationTest, SerializesAsOAIMessage) {
  ContentBlockSerializationTestParam params = GetParam();

  // Compute expected text at runtime
  std::string expected_text;
  if (params.localized_text) {
    expected_text =
        params.localized_text->format_arg
            ? l10n_util::GetStringFUTF8(
                  params.localized_text->message_id,
                  base::UTF8ToUTF16(*params.localized_text->format_arg))
            : l10n_util::GetStringUTF8(params.localized_text->message_id);
  } else if (params.literal_text) {
    expected_text = *params.literal_text;
  }

  auto content_block = params.content_factory.Run();
  base::Value::Dict expected_msg = base::Value::Dict().Set("role", "user");
  base::Value::Dict expected_content_block =
      base::Value::Dict().Set("type", params.expected_type);

  if (content_block.type == ExtendedContentBlockType::kImage) {
    const ImageContent* img = std::get_if<ImageContent>(&content_block.data);
    ASSERT_TRUE(img);

    base::Value::Dict image_url_dict;
    image_url_dict.Set("url", img->image_url.url);
    if (img->image_url.detail) {
      image_url_dict.Set("detail", *img->image_url.detail);
    }

    expected_content_block.Set("image_url", std::move(image_url_dict));
  } else {
    expected_content_block.Set("text", expected_text);
  }
  expected_msg.Set(
      "content", base::Value::List().Append(std::move(expected_content_block)));

  std::vector<OAIMessage> messages;
  OAIMessage message;
  message.role = "user";
  message.content.emplace_back(std::move(content_block));
  messages.push_back(std::move(message));

  auto serialized = OAIAPIClient::SerializeOAIMessages(std::move(messages));

  ASSERT_EQ(serialized.size(), 1u);
  const base::Value::Dict* message_dict = serialized[0].GetIfDict();
  ASSERT_TRUE(message_dict);
  EXPECT_EQ(*message_dict, expected_msg);
}

// Adding any new types into ExtendedContentBlock enum should update this test.
INSTANTIATE_TEST_SUITE_P(
    ,
    ContentBlockSerializationTest,
    testing::Values(
        ContentBlockSerializationTestParam{
            "Text", base::BindRepeating([]() {
              return ExtendedContentBlock(ExtendedContentBlockType::kText,
                                          TextContent{kTestContent});
            }),
            "text", std::nullopt, kTestContent},
        ContentBlockSerializationTestParam{
            "Image", base::BindRepeating([]() {
              ImageContent img;
              img.image_url.url = kTestImageUrl;
              img.image_url.detail = "high";
              return ExtendedContentBlock(ExtendedContentBlockType::kImage,
                                          std::move(img));
            }),
            "image_url", std::nullopt, std::nullopt},
        ContentBlockSerializationTestParam{
            "PageExcerpt", base::BindRepeating([]() {
              return ExtendedContentBlock(
                  ExtendedContentBlockType::kPageExcerpt,
                  TextContent{kTestContent});
            }),
            "text",
            LocalizedText{IDS_AI_CHAT_LLAMA2_SELECTED_TEXT_PROMPT_SEGMENT,
                          kTestContent},
            std::nullopt},
        ContentBlockSerializationTestParam{
            "ChangeTone", base::BindRepeating([]() {
              return ExtendedContentBlock(ExtendedContentBlockType::kChangeTone,
                                          ChangeToneContent{"casual"});
            }),
            "text",
            LocalizedText{IDS_AI_CHAT_QUESTION_CHANGE_TONE_TEMPLATE, "casual"},
            std::nullopt},
        ContentBlockSerializationTestParam{
            "Paraphrase", base::BindRepeating([]() {
              return ExtendedContentBlock(ExtendedContentBlockType::kParaphrase,
                                          TextContent{""});
            }),
            "text", LocalizedText{IDS_AI_CHAT_QUESTION_PARAPHRASE},
            std::nullopt},
        ContentBlockSerializationTestParam{
            "Improve", base::BindRepeating([]() {
              return ExtendedContentBlock(ExtendedContentBlockType::kImprove,
                                          TextContent{""});
            }),
            "text", LocalizedText{IDS_AI_CHAT_QUESTION_IMPROVE}, std::nullopt},
        ContentBlockSerializationTestParam{
            "Shorten", base::BindRepeating([]() {
              return ExtendedContentBlock(ExtendedContentBlockType::kShorten,
                                          TextContent{""});
            }),
            "text", LocalizedText{IDS_AI_CHAT_QUESTION_SHORTEN}, std::nullopt},
        ContentBlockSerializationTestParam{
            "Expand", base::BindRepeating([]() {
              return ExtendedContentBlock(ExtendedContentBlockType::kExpand,
                                          TextContent{""});
            }),
            "text", LocalizedText{IDS_AI_CHAT_QUESTION_EXPAND}, std::nullopt}),
    [](const testing::TestParamInfo<ContentBlockSerializationTestParam>& info) {
      return info.param.name;
    });

class OAIAPIInvalidResponseTest
    : public OAIAPIUnitTest,
      public ::testing::WithParamInterface<std::string> {};

TEST_P(OAIAPIInvalidResponseTest,
       InvalidResponse_NoCallbacksTriggeredOrEmptyCompletion) {
  mojom::CustomModelOptionsPtr model_options = mojom::CustomModelOptions::New(
      "test_api_key", 0, 0, 0, "test_system_prompt", GURL("https://test.com"),
      "test_model");

  const std::string invalid_server_response = GetParam();

  base::RunLoop run_loop;
  testing::StrictMock<MockCallbacks> mock_callbacks;
  MockAPIRequestHelper* mock_request_helper =
      client_->GetMockAPIRequestHelper();

  EXPECT_CALL(*mock_request_helper, RequestSSE(_, _, _, _, _, _, _, _))
      .WillOnce([&](auto, auto, auto, auto,
                    DataReceivedCallback data_received_callback,
                    ResultCallback result_callback, auto, auto) {
        // Simulate data chunk received
        base::Value maybe_val = ParseOrStringValue(invalid_server_response);
        data_received_callback.Run(base::ok(std::move(maybe_val)));

        // Simulate final callback
        base::Value maybe_val_final =
            ParseOrStringValue(invalid_server_response);
        std::move(result_callback)
            .Run(api_request_helper::APIRequestResult(
                200, std::move(maybe_val_final), {}, net::OK, GURL()));

        run_loop.Quit();
        return Ticket();
      });

  // For invalid payloads, we expect no callbacks from OnDataReceived
  EXPECT_CALL(mock_callbacks, OnDataReceived(_)).Times(0);

  // For invalid 200 OK payloads, we expect an empty completion from OnCompleted
  EXPECT_CALL(mock_callbacks, OnCompleted(_))
      .WillOnce([](GenerationResult result) {
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result->event);
        ASSERT_TRUE(result->event->is_completion_event());
        EXPECT_EQ(result->event->get_completion_event()->completion, "");
      });

  // Begin request
  client_->PerformRequest(
      *model_options, base::Value::List(),
      base::BindRepeating(&MockCallbacks::OnDataReceived,
                          base::Unretained(&mock_callbacks)),
      base::BindOnce(&MockCallbacks::OnCompleted,
                     base::Unretained(&mock_callbacks)));

  run_loop.Run();

  testing::Mock::VerifyAndClearExpectations(mock_request_helper);
  testing::Mock::VerifyAndClearExpectations(&mock_callbacks);
}

// A set of invalid responses that should not trigger any callbacks
INSTANTIATE_TEST_SUITE_P(
    OAIAPIInvalidResponseScenarios,
    OAIAPIInvalidResponseTest,
    ::testing::Values(
        // aaaaaaaaaaaaaaa
        "aaaaaaaaaaaaaaaaa",
        // {"invalid": "json"}
        R"({"invalid": "json"})",
        // {choices: []}
        R"({"choices": []})",
        // {"choices": [{"message": {"content": []}]}
        R"({"choices": [{"message": {"content": []}}]})",
        // Empty JSON object
        R"({})",
        // Malformed JSON
        R"({"choices": [)",
        // Unexpected data types
        R"({"choices": "unexpected_string"})",
        // Nested invalid JSON
        R"({"choices": [{"message": {"content": {"nested": "invalid"}}}]})",
        // Valid JSON with missing fields
        R"({"choices": [{"index": 0}]})"));

}  // namespace ai_chat
