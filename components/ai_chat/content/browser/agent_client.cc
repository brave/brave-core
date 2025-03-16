// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/agent_client.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/string_escape.h"
#include "base/strings/stringprintf.h"
#include "base/strings/to_string.h"
#include "base/types/expected.h"
#include "brave/browser/ui/brave_browser_window.h"
#include "brave/components/ai_chat/content/browser/ai_chat_cursor.h"
#include "brave/components/ai_chat/content/browser/build_devtools_key_event_params.h"
#include "brave/components/ai_chat/content/browser/dom_nodes_xml_string.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

namespace {
constexpr size_t kForcedWidth = 1024;
constexpr size_t kForcedHeight = 768;

void ScrollAtPoint(content::WebContents* web_contents,
                   gfx::Point position,
                   int delta_x,
                   int delta_y) {
  if (!web_contents) {
    return;
  }

  std::string script =
      base::StringPrintf(R"JS((function() {
    let target = document.elementFromPoint(%d, %d);
    if (!target) { return }

    while (target && target !== document.body &&
        target !== document.documentElement &&
        target.scrollHeight <= target.clientHeight) {
      target = target.parentElement
    }
    if (target) {
      target.scrollBy(%d, %d)
    }
  })())JS",
                         position.x(), position.y(), delta_x, delta_y);

  web_contents->GetPrimaryMainFrame()->ExecuteJavaScriptInIsolatedWorld(
      base::ASCIIToUTF16(script), base::NullCallback(),
      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
}

std::optional<gfx::Point> ExtractCoordinatesFromMouseAction(
    const base::Value::Dict& input) {
  auto* coordinates = input.FindList("coordinate");
  if (!coordinates || coordinates->size() != 2 || !(*coordinates)[0].is_int() ||
      !(*coordinates)[1].is_int()) {
    return std::nullopt;
  }

  return gfx::Point((*coordinates)[0].GetInt(), (*coordinates)[1].GetInt());
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
  return "computer_20250124";
}

std::optional<base::Value::Dict> AgentClient::extra_params() const {
  base::Value::Dict dict;
  dict.Set("display_width_px", (int)kForcedWidth);
  dict.Set("display_height_px", (int)kForcedHeight);
  // dict.Set("display_number", 1);
  return dict;
}

void AgentClient::UseTool(const std::string& input_json,
                          Tool::UseToolCallback callback) {
  GetDomTree();
  DVLOG(4) << __func__ << " input_json = " << input_json;
  auto json_message = base::JSONReader::Read(input_json);
  if (!json_message || !json_message->is_dict()) {
    DVLOG(0) << "Failed to parse input JSON: " << input_json;
    std::move(callback).Run(
        CreateContentBlocksForText("Error - failed to parse input JSON"));
    return;
  }
  base::Value::Dict& input = json_message->GetDict();
  std::string* action = input.FindString("action");
  if (!action) {
    DVLOG(0) << "No action found in input_json: " << input_json;
    std::move(callback).Run(
        CreateContentBlocksForText("Error - no action string found"));
    return;
  }

  if (*action == "cursor_position") {
    std::move(callback).Run(CreateContentBlocksForText(base::StringPrintf(
        "x=%d, y=%d", mouse_position_.x(), mouse_position_.y())));
    return;
  }

  PrepareForAgentActions();

  if (*action == "screenshot") {
    AgentClient::CaptureScreenshot(std::move(callback));
  } else if (*action == "type") {
    auto* input_text = input.FindString("text");
    if (!input_text) {
      DVLOG(0) << "No text found in input_json: " << input_json;
      std::move(callback).Run(
          CreateContentBlocksForText("Error - no text string found in input"));
      return;
    }
    AgentClient::TypeText(*input_text, std::move(callback));
  } else if (*action == "mouse_move") {
    auto coordinates = ExtractCoordinatesFromMouseAction(input);
    if (!coordinates.has_value()) {
      DVLOG(0) << "Invalid coordinates: " << input_json;
      std::move(callback).Run(CreateContentBlocksForText(
          "Error - invalid coordinates found in input"));
      return;
    }

    UpdateMousePosition(coordinates.value());
    Execute(
        "Input.dispatchMouseEvent",
        R"({
    "type": "mouseMoved",
    "x": )" +
            base::ToString(mouse_position_.x()) +
            R"(,
    "y": )" +
            base::ToString(mouse_position_.y()) +
            R"(
  })",
        base::BindOnce(
            [](Tool::UseToolCallback callback, MessageResult result_raw) {
              if (!result_raw.has_value()) {
                std::move(callback).Run(CreateContentBlocksForText(("error")));
                return;
              }
              auto result = base::JSONReader::Read(result_raw.value());
              if (!result.has_value() || !result->is_dict()) {
                std::move(callback).Run(CreateContentBlocksForText(("error")));
                return;
              }
              std::move(callback).Run(CreateContentBlocksForText(("success")));
            },
            std::move(callback)));
  } else if (*action == "left_click") {
    auto coordinates = ExtractCoordinatesFromMouseAction(input);
    if (!coordinates.has_value()) {
      DVLOG(0) << "Invalid coordinates: " << input_json;
      std::move(callback).Run(CreateContentBlocksForText(
          "Error - invalid coordinates found in input"));
      return;
    }

    UpdateMousePosition(coordinates.value());

    // TODO: AgentClient::LeftClick
    auto agent_action = base::BindOnce(
        [](base::WeakPtr<AgentClient> instance, base::OnceClosure on_done) {
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
                            base::BindOnce(
                                [](base::OnceClosure on_done,
                                   MessageResult result) {
                                  std::move(on_done).Run();
                                },
                                std::move(on_done)));
        },
        weak_factory_.GetWeakPtr());

    PerformPossiblyNavigatingAction(std::move(agent_action),
                                    std::move(callback));
  } else if (*action == "key") {
    auto* key = input.FindString("text");
    if (!key) {
      DVLOG(0) << "No key found in input_json: " << input_json;
      std::move(callback).Run(
          CreateContentBlocksForText("error - no key found in input"));
      return;
    }
    auto agent_action = base::BindOnce(
        [](base::WeakPtr<AgentClient> instance, const std::string& in_key,
           base::OnceClosure on_done) {
          // TODO(petemill): Get an Accelerator via
          // extensions/common/command.cc's ParseImpl and then one of:
          // devtools_agent_host_->GetWebContents()->GetRenderViewHost()->GetWidget()->ForwardKeyboardEvent(const
          // input::NativeWebKeyboardEvent &key_event)
          // views::Widget::GetWidgetForNativeView(devtools_agent_host_->GetWebContents()->GetNativeView())->GetContentsView()->AcceleratorPressed({});
          auto params = BuildDevToolsKeyEventParams(in_key);
          //  "nativeVirtualKeyCode": )" +
          //         base::ToString(params.native_virtual_key_code) + R"(,
          //                         "code": ")" +
          // params.dom_code_string + R"(",
          instance->Execute(
              "Input.dispatchKeyEvent",
              R"({
                                "type": "rawKeyDown",
                                "windowsVirtualKeyCode": )" +
                  base::ToString(params.windows_native_virtual_key_code) + R"(,
                                "modifiers": )" +
                  base::ToString(params.modifiers) + R"(
                              })",
              base::DoNothing());

          instance->Execute(
              "Input.dispatchKeyEvent",
              R"({
                                "type": "keyUp",
                                "windowsVirtualKeyCode": )" +
                  base::ToString(params.windows_native_virtual_key_code) + R"(,
                                "nativeVirtualKeyCode": )" +
                  base::ToString(params.native_virtual_key_code) + R"(,
                                "code": ")" +
                  params.dom_code_string + R"(",
                                "modifiers": )" +
                  base::ToString(params.modifiers) + R"(
                              })",
              base::BindOnce(
                  [](base::OnceClosure on_done, MessageResult result) {
                    std::move(on_done).Run();
                  },
                  std::move(on_done)));
        },
        weak_factory_.GetWeakPtr(), *key);
    PerformPossiblyNavigatingAction(std::move(agent_action),
                                    std::move(callback));
  } else if (*action == "scroll") {
    std::string* direction = input.FindString("scroll_direction");
    if (!direction) {
      DVLOG(0) << "No scroll_direction found in input_json: " << input_json;
      std::move(callback).Run(CreateContentBlocksForText(
          "error - no scroll_direction found in input"));
      return;
    }
    std::optional<int> scroll_amount = input.FindInt("scroll_amount");
    if (!scroll_amount) {
      DVLOG(0) << "No scroll_amount found in input_json: " << input_json;
      std::move(callback).Run(CreateContentBlocksForText(
          "error - no scroll_amount found in input"));
      return;
    }

    // Calculate delta_x and delta_y. Assume a "click" is 16px
    scroll_amount = *scroll_amount * 16;
    int delta_x = 0;
    int delta_y = 0;
    if (*direction == "down") {
      delta_y = *scroll_amount;
    } else if (*direction == "up") {
      delta_y = -(*scroll_amount);
    } else if (*direction == "right") {
      delta_x = *scroll_amount;
    } else if (*direction == "left") {
      delta_x = -(*scroll_amount);
    } else {
      DVLOG(0) << "Invalid scroll_direction: " << *direction;
      std::move(callback).Run(CreateContentBlocksForText(
          "error - invalid scroll_direction found in input"));
      return;
    }

    auto coordinates = ExtractCoordinatesFromMouseAction(input);
    if (!coordinates.has_value()) {
      DVLOG(0) << "Invalid coordinates: " << input_json;
      std::move(callback).Run(CreateContentBlocksForText(
          "error - invalid coordinates found in input"));
      return;
    }

    UpdateMousePosition(coordinates.value());

    ScrollAtPoint(devtools_agent_host_->GetWebContents(), mouse_position_,
                  delta_x, delta_y);

    AgentClient::CaptureScreenshot(std::move(callback));
  } else {
    DVLOG(0) << "Unknown action: " << action;
    std::move(callback).Run(
        CreateContentBlocksForText("Error - unknown action found in input"));
    return;
  }
}

void AgentClient::GetDomTree() {
  devtools_agent_host_->GetWebContents()->RequestAXTreeSnapshot(
      base::BindOnce(&AgentClient::OnAXTreeSnapshot,
                     weak_factory_.GetWeakPtr()),
      ui::AXMode::kWebContents | ui::AXMode::kScreenReader |
          ui::AXMode::kLabelImages,
      /* max_nodes= */ 9000, /* timeout= */ base::Seconds(2),
      content::WebContents::AXTreeSnapshotPolicy::kSameOriginDirectDescendants);
}

void AgentClient::OnAXTreeSnapshot(ui::AXTreeUpdate& tree) {
  GetDomNodesXmlString(tree);
}

void AgentClient::CaptureScreenshot(Tool::UseToolCallback callback) {
  Execute(
      "Page.captureScreenshot",
      R"({
            "format": "webp",
            "quality": 75
          })",
      base::BindOnce(
          [](Tool::UseToolCallback callback, MessageResult result_raw) {
            if (!result_raw.has_value()) {
              std::move(callback).Run(CreateContentBlocksForText(("error")));
              return;
            }
            auto result = base::JSONReader::Read(result_raw.value());
            if (!result.has_value() || !result->is_dict()) {
              std::move(callback).Run(CreateContentBlocksForText(("error")));
              return;
            }
            base::Value::Dict& result_dict = result->GetDict();
            std::string* image_data = result_dict.FindString("data");
            if (!image_data) {
              std::move(callback).Run(CreateContentBlocksForText(("error")));
              return;
            }

            std::move(callback).Run(CreateContentBlocksForImage(
                "data:image/webp;base64," + *image_data));
          },
          std::move(callback)));
}

void AgentClient::TypeText(std::string_view text,
                           Tool::UseToolCallback callback) {
  std::string escaped;
  base::EscapeJSONString(text, false, &escaped);
  Execute("Input.insertText",
          R"({
    "text": ")" +
              escaped +
              R"("
  })",
          base::BindOnce(
              [](Tool::UseToolCallback callback, MessageResult result) {
                if (!result.has_value()) {
                  std::move(callback).Run(CreateContentBlocksForText(
                      "There was an error, please try again"));
                  return;
                }
                std::move(callback).Run(CreateContentBlocksForText("success"));
              },
              std::move(callback)));
}

void AgentClient::UpdateMousePosition(const gfx::Point& position) {
  mouse_position_ = position;
  cursor_overlay_->MoveCursorTo(mouse_position_.x(), mouse_position_.y());
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
      this, base::as_bytes(base::span(json_command)));
}

void AgentClient::DispatchProtocolMessage(
    content::DevToolsAgentHost* agent_host,
    base::span<const uint8_t> message_bytes) {
  std::string_view message_raw(
      reinterpret_cast<const char*>(message_bytes.data()),
      message_bytes.size());
  // LOG(ERROR) << __func__ << " message_raw = " << message_raw;
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
    std::move(callback).Run(base::unexpected(""));
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

void AgentClient::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  if (pending_navigation_callback_.is_null()) {
    return;
  }
  pending_navigation_complete_ = false;
}

void AgentClient::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (pending_navigation_callback_.is_null()) {
    return;
  }
  pending_navigation_complete_ = true;
  MaybeFinishPossiblyNavigatingAction();
}

void AgentClient::DidFirstVisuallyNonEmptyPaint() {
  if (pending_navigation_callback_.is_null()) {
    return;
  }
  pending_navigation_visually_painted_ = true;
  MaybeFinishPossiblyNavigatingAction();
}

void AgentClient::PrepareForAgentActions() {
  devtools_agent_host_->AttachClient(this);
  if (!cursor_overlay_) {
    // TODO: use a tab helper to show the cursor so that
    // the browser can only show it when the tab is active
    cursor_overlay_ = std::make_unique<AIChatCursorOverlay>(
        devtools_agent_host_->GetWebContents());
  }
  cursor_overlay_->ShowCursor();

  devtools_agent_host_->Activate();
  devtools_agent_host_->GetWebContents()->Focus();

  if (!has_overriden_metrics_) {
    has_overriden_metrics_ = true;
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
            base::DoNothing());
  } else {
    Execute("Page.bringToFront", "{}", base::DoNothing());
  }
}

void AgentClient::PerformPossiblyNavigatingAction(
    base::OnceCallback<void(base::OnceClosure)> action,
    Tool::UseToolCallback callback) {
  pending_navigation_callback_ = std::move(callback);
  pending_navigation_complete_ = std::nullopt;
  pending_navigation_visually_painted_ = false;
  // Run the action and when it completes, check if a navigation started
  std::move(action).Run(base::BindOnce(
      [](base::OnceClosure delayed_task) {
        base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
            FROM_HERE, std::move(delayed_task), base::Milliseconds(500));
      },
      base::BindOnce(&AgentClient::MaybeFinishPossiblyNavigatingAction,
                     weak_factory_.GetWeakPtr())));
}

void AgentClient::MaybeFinishPossiblyNavigatingAction() {
  // Run the task if a navigation did not start or if it did and it completed
  if (!pending_navigation_complete_.has_value() ||
      (pending_navigation_complete_.value() &&
       pending_navigation_visually_painted_)) {
    if (pending_navigation_callback_.is_null()) {
      return;
    }
    CaptureScreenshot(std::move(pending_navigation_callback_));
    pending_navigation_callback_.Reset();
    pending_navigation_complete_ = std::nullopt;
    pending_navigation_visually_painted_ = false;
  }
}

}  // namespace ai_chat
