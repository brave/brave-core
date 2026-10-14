// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_key_util.h"

#include <ostream>  // IWYU pragma: keep

#include "base/notreached.h"

namespace gestures {

namespace {

std::string ContentTypeKey(SmartGestureContentType content_type) {
  switch (content_type) {
    case SmartGestureContentType::kUndefined:
      NOTREACHED() << "SmartGestureContentInfo was never assigned a real "
                      "content type before building its key.";
    case SmartGestureContentType::kLink:
      return "link";
    case SmartGestureContentType::kImage:
      return "image";
    case SmartGestureContentType::kSelectedText:
      return "selected_text";
  }
}

std::string DirectionTypeKey(SmartGestureDirectionType direction_type) {
  switch (direction_type) {
    case SmartGestureDirectionType::kUp:
      return "up";
    case SmartGestureDirectionType::kDown:
      return "down";
    case SmartGestureDirectionType::kLeft:
      return "left";
    case SmartGestureDirectionType::kRight:
      return "right";
  }
}

}  // namespace

std::string SmartGestureKey(SmartGestureContentType content_type,
                            SmartGestureDirectionType direction_type) {
  return ContentTypeKey(content_type) + ":" + DirectionTypeKey(direction_type);
}

}  // namespace gestures
