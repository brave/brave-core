/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/page/frame_tree.h"

#define ExperimentalSetNulledName(...)                                \
  GetName();                                                          \
  if (!frame_->origin_for_clear_window_name_check_.IsSameOriginWith(  \
          frame_->DomWindow()->GetSecurityOrigin()->ToUrlOrigin())) { \
    frame_->Tree().ExperimentalSetNulledName(__VA_ARGS__);            \
  }                                                                   \
  frame_->origin_for_clear_window_name_check_ = url::Origin();

#include <third_party/blink/renderer/core/loader/document_loader.cc>

#undef ExperimentalSetNulledName
