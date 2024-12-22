// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/agent_client.h"

#include <cstddef>
#include <string>

#include "base/base64.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/to_string.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/content/browser/ai_chat_cursor.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "include/codec/SkCodec.h"
#include "include/codec/SkPngDecoder.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkStream.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/encode/SkPngEncoder.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/codec/webp_codec.h"

namespace ai_chat {

namespace {
constexpr size_t kForcedWidth = 1024;
constexpr size_t kForcedHeight = 768;

std::string AddBorderToBase64Image(const std::string& base64_input,
                                   int border_size,
                                   SkColor border_color) {
  // 1. Base64-decode into raw bytes
  std::string decoded_bytes;
  if (!base::Base64Decode(base64_input, &decoded_bytes)) {
    // Handle error (return empty or log)
    return std::string();
  }

  // 2. Create SkData from the raw bytes
  sk_sp<SkData> sk_data =
      SkData::MakeWithCopy(decoded_bytes.data(), decoded_bytes.size());
  if (!sk_data) {
    LOG(ERROR) << "Failed to create SkData";
    // Handle error
    return std::string();
  }

  SkCodec::Result decode_result;
  std::unique_ptr<SkCodec> codec =
      SkPngDecoder::Decode(sk_data, &decode_result);
  if (decode_result != SkCodec::Result::kSuccess) {
    LOG(ERROR) << "Failed to create SkCodec: " << decode_result;
    return std::string();
  }

  SkImageInfo image_info = codec->getInfo()
                               .makeColorType(kN32_SkColorType)
                               .makeAlphaType(kPremul_SkAlphaType);

  SkBitmap bitmap;
  if (!bitmap.tryAllocPixels(image_info)) {
    LOG(ERROR) << "Failed to allocate pixels";
    // Handle error
    return std::string();
  }

  // Actually decode into the bitmap
  const SkCodec::Result result =
      codec->getPixels(image_info, bitmap.getPixels(), bitmap.rowBytes());
  if (result != SkCodec::kSuccess) {
    // Handle error
    LOG(ERROR) << "Failed to decode image";
    return std::string();
  }

  SkCanvas canvas(bitmap, SkSurfaceProps{});

  SkPaint paint;
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setColor(border_color);
  paint.setStrokeWidth(border_size);  // e.g. 2px for the border line

  SkRect border_rect =
      SkRect::MakeXYWH(static_cast<SkScalar>(0), static_cast<SkScalar>(0),
                       static_cast<SkScalar>(bitmap.width()),
                       static_cast<SkScalar>(bitmap.height()));
  canvas.drawRect(border_rect, paint);

  // PNG (too big):
  // SkPixmap pixmap;
  // if (!bitmap.peekPixels(&pixmap)) {
  //   LOG(ERROR) << "Failed to peekPixels";
  //   return std::string();
  // }

  // SkDynamicMemoryWStream wstream;
  // SkPngEncoder::Options png_options;
  // png_options.fFilterFlags = SkPngEncoder::FilterFlag::kNone;
  // png_options.fZLibLevel   = 9;  // compression level

  // if (!SkPngEncoder::Encode(&wstream, pixmap, png_options)) {
  //   // Handle error
  //   LOG(ERROR) << "Failed to encode PNG";
  //   return std::string();
  // }

  // sk_sp<SkData> encoded = wstream.detachAsData();

  auto encoded = gfx::WebpCodec::Encode(bitmap, 75);
  if (!encoded.has_value()) {
    LOG(ERROR) << "Failed to encode WebP";
    return std::string();
  }

  std::string encoded_png_str(reinterpret_cast<const char*>(encoded->data()),
                              encoded->size());

  return base::Base64Encode(encoded_png_str);
}
}  // namespace

AgentClient::AgentClient(content::WebContents* web_contents)
    : devtools_agent_host_(
          content::DevToolsAgentHost::GetOrCreateFor(web_contents)) {}

AgentClient::~AgentClient() {
  devtools_agent_host_->DetachClient(this);
}

std::string_view AgentClient::name() const {
  return "computer";
}

std::string_view AgentClient::description() const {
  return "";
}

std::string_view AgentClient::type() const {
  return "computer_20241022";
}

std::optional<std::string> AgentClient::GetInputSchemaJson() const {
  auto schema = R"({
          "display_width_px": )" +
                base::ToString(kForcedWidth) + R"(,
          "display_height_px": )" +
                base::ToString(kForcedHeight) + R"(
    })";
  LOG(ERROR) << __func__ << " schema = " << schema;
  return schema;
}

void AgentClient::UseTool(
    const std::string& input_json,
    base::OnceCallback<void(std::optional<std::string_view>)> callback) {
  LOG(ERROR) << __func__ << " input_json = " << input_json;
  DVLOG(4) << __func__ << " input_json = " << input_json;
  auto json_message = base::JSONReader::Read(input_json);
  if (!json_message || !json_message->is_dict()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  base::Value::Dict& input = json_message->GetDict();
  std::string* action = input.FindString("action");
  if (!action) {
    DLOG(ERROR) << "No action found in input_json: " << input_json;
    std::move(callback).Run(std::nullopt);
    return;
  }

  if (*action == "cursor_position") {
    std::move(callback).Run(R"([{
      "type": "text",
        "x": )" + base::ToString(mouse_position_.x()) + R"(,
        "y": )" + base::ToString(mouse_position_.y()) + R"(,
      }])");
    return;
  }

  devtools_agent_host_->AttachClient(this);

  if (!cursor_overlay_) {
    cursor_overlay_ = std::make_unique<AIChatCursorOverlay>(
        devtools_agent_host_->GetWebContents());
  }
  cursor_overlay_->ShowCursor();

  auto message_callback =
      base::BindOnce(&AgentClient::OnMessageForToolUseComplete,
                     base::Unretained(this), std::move(callback));
  base::OnceCallback<void()> do_action;

  if (*action == "screenshot") {
    do_action =
        base::BindOnce(&AgentClient::CaptureScreenshot, base::Unretained(this),
                       std::move(message_callback));
  } else if (*action == "type") {
    do_action = base::BindOnce(&AgentClient::TypeText, base::Unretained(this),
                               input.FindString("text") ? *input.FindString("text")
                                                        : "",
                               std::move(message_callback));
  } else if (*action == "mouse_move") {
    auto* coordinates = input.FindList("coordinate");
    if (!coordinates || coordinates->size() != 2) {
      DLOG(ERROR) << "Invalid coordinates: " << input_json;
      std::move(callback).Run(std::nullopt);
      return;
    }
    mouse_position_ =
        gfx::Point((*coordinates)[0].GetInt(), (*coordinates)[1].GetInt());
    cursor_overlay_->MoveCursorTo(mouse_position_.x(), mouse_position_.y());
    do_action = base::BindOnce(
        [](AgentClient* instance, MessageCallback callback) {
          instance->Execute("Input.dispatchMouseEvent",
                            R"({
        "type": "mouseMoved",
        "x": )" + base::ToString(instance->mouse_position_.x()) +
                                R"(,
        "y": )" + base::ToString(instance->mouse_position_.y()) +
                                R"(
      })",
                            std::move(callback));
        },
        base::Unretained(this), std::move(message_callback));
  } else if (*action == "left_click") {
    do_action = base::BindOnce(
        [](AgentClient* instance, MessageCallback callback) {
          instance->Execute("Input.dispatchMouseEvent",
                            R"({
        "type": "mousePressed",
        "x": )" + base::ToString(instance->mouse_position_.x()) +
                                R"(,
        "y": )" + base::ToString(instance->mouse_position_.y()) +
                                R"(,
        "button": "left",
        "clickCount": 1
      })",
                            base::DoNothing());

          instance->Execute("Input.dispatchMouseEvent",
                            R"({
        "type": "mouseReleased",
        "x": )" + base::ToString(instance->mouse_position_.x()) +
                                R"(,
        "y": )" + base::ToString(instance->mouse_position_.y()) +
                                R"(,
        "button": "left",
        "clickCount": 1
      })",
                            std::move(callback));
        },
        base::Unretained(this), std::move(message_callback));
  } else if (*action == "key") {
    auto* key = input.FindString("text");
    if (!key) {
      DLOG(ERROR) << "No key found in input_json: " << input_json;
      std::move(callback).Run(std::nullopt);
      return;
    }
    do_action = base::BindOnce(
        [](AgentClient* instance, const std::string& key,
           MessageCallback callback) {
          auto code = key;
          if (key == "Page_Down") {
            code = "Enter";
          }
          instance->Execute("Input.dispatchKeyEvent",
                            R"({
    "type": "keyDown",
    "windowsVirtualKeyCode": 34,
    "nativeVirtualKeyCode": 34,
    "code": "PageDown",
    "key": "PageDown",
    "modifiers": 0
      })",
                            base::DoNothing());

          instance->Execute("Input.dispatchKeyEvent",
                            R"({
    "type": "keyUp",
    "windowsVirtualKeyCode": 34,
    "nativeVirtualKeyCode": 34,
    "code": "PageDown",
    "key": "PageDown",
    "modifiers": 0
      })",
                            std::move(callback));
        },
        base::Unretained(this), *key, std::move(message_callback));
  } else {
    DLOG(ERROR) << "Unknown action: " << action;
    std::move(callback).Run(std::nullopt);
    return;
  }

  // We must set the viewport size and scale first
  Execute("Emulation.setDeviceMetricsOverride",
          R"({
    "width": )" +
              base::ToString(kForcedWidth) + R"(,
    "height": )" +
              base::ToString(kForcedHeight) + R"(,
    "deviceScaleFactor": 1,
    "mobile": false
  })",
          base::BindOnce(
              [](base::OnceCallback<void()> do_action,
                 MessageResult device_metrics_result) {
                std::move(do_action).Run();
              },
              std::move(do_action)));
}

void AgentClient::OnMessageForToolUseComplete(
    base::OnceCallback<void(std::optional<std::string_view>)> tool_use_callback,
    MessageResult result) {
  if (!result.has_value()) {
    std::move(tool_use_callback).Run("error");
    return;
  }
  std::move(tool_use_callback).Run(result.value());
}

void AgentClient::CaptureScreenshot(MessageCallback callback) {
  // Capture screenshot
  // "quality": 70,
  //           "clip:": {
  //             "x": 0,
  //             "y": 0,
  //             "width": )" +
  //             base::ToString(kForcedWidth) + R"(,
  //             "height": )" +
  //             base::ToString(kForcedHeight) + R"(,
  //             "scale": 1.0
  //           }
  Execute("Page.captureScreenshot",
          R"({
            "format": "png"
          })",
          base::BindOnce(
              [](MessageCallback callback, MessageResult result_raw) {
                if (!result_raw.has_value()) {
                  std::move(callback).Run(base::unexpected("error"));
                  return;
                }
                auto result = base::JSONReader::Read(result_raw.value());
                if (!result.has_value() || !result->is_dict()) {
                  std::move(callback).Run(base::unexpected("error"));
                  return;
                }
                base::Value::Dict& result_dict = result->GetDict();
                std::string* image_data = result_dict.FindString("data");
                if (!image_data) {
                  std::move(callback).Run(base::unexpected("error"));
                  return;
                }
                // Add a border to the image
                std::string new_image =
                    AddBorderToBase64Image(*image_data, 6, SK_ColorGRAY);
                DLOG(ERROR) << __func__ << " new_image = " << new_image;

                // "source": {
                //   "type": "base64",
                //   "media_type": "image/webp",
                //   "data": ")" + new_image + R"("
                // }

                // This format is not well documented by Anthropic, but it's
                // what the assistant expects after trial and error:
                auto response = R"([{
                  "type": "image_url",
                  "image_url": "data:image/webp;base64,)" +
                                new_image + R"("
                }])";

                std::move(callback).Run(base::ok(response));
              },
              std::move(callback)));
}

void AgentClient::TypeText(std::string_view text, MessageCallback callback) {
  Execute("Input.insertText",
          R"({
    "text": ")" + std::string(text) +
              R"("
  })",
          std::move(callback));
}

void AgentClient::Execute(std::string_view method,
                          std::string_view params,
                          MessageCallback callback) {
  auto command_id = request_id_++;
  message_callbacks_.insert_or_assign(command_id, std::move(callback));
  std::string json_command = R"({
    "id": )" + base::ToString(command_id) +
                             R"(,
    "method": ")" + std::string(method) +
                             R"(",
    "params": )" + std::string(params) +
                             R"(
  })";

  LOG(ERROR) << __func__ << " json_command = " << json_command;

  devtools_agent_host_->AttachClient(this);
  devtools_agent_host_->DispatchProtocolMessage(
      this, base::as_bytes(base::make_span(json_command)));
}

void AgentClient::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host,
    base::span<const uint8_t> message_bytes) {
  std::string_view message_raw(
      reinterpret_cast<const char*>(message_bytes.data()),
      message_bytes.size());
  LOG(ERROR) << __func__ << " message_raw = " << message_raw;
  std::optional<base::Value> message = base::JSONReader::Read(message_raw);
  if (!message || !message->is_dict()) {
    DLOG(ERROR) << "Failed to parse message: " << message_raw;
    return;
  }

  base::Value::Dict& message_dict = message->GetDict();

  auto id_param = message_dict.FindInt("id");
  if (!id_param) {
    DLOG(ERROR) << "No id found in message: " << message_raw;
    return;
  }

  int id = *id_param;

  if (!message_callbacks_.contains(id)) {
    DLOG(ERROR) << "No callback found for request_id: " << id;
    return;
  }
  auto callback = std::move(message_callbacks_[id]);
  message_callbacks_.erase(id);

  base::Value::Dict* result = message_dict.FindDict("result");
  std::string result_json;
  if (!result || !base::JSONWriter::Write(*result, &result_json)) {
    std::move(callback).Run(base::ok(""));
    return;
  }

  std::move(callback).Run(base::ok(result_json));
  return;
}

void AgentClient::AgentHostClosed(content::DevToolsAgentHost* agent_host) {
  LOG(ERROR) << __func__;
}

bool AgentClient::MayAttachToRenderFrameHost(
    content::RenderFrameHost* render_frame_host) {
  return true;
}
bool AgentClient::IsTrusted() {
  return true;
}

}  // namespace ai_chat
