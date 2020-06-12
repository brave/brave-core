/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/webaudio/analyser_node.h"

#include "third_party/blink/renderer/core/frame/local_dom_window.h"

#define BRAVE_ANALYSERHANDLER_CONSTRUCTOR                 \
  ExecutionContext* context = node.GetExecutionContext(); \
  analyser_.audio_farbling_callback_ =                    \
      brave::BraveSessionCache::From(                     \
          *(To<LocalDOMWindow>(context)->document()))     \
          .GetAudioFarblingCallback(                      \
              To<LocalDOMWindow>(context)->document()->GetFrame());

#include "../../../../../../../third_party/blink/renderer/modules/webaudio/analyser_node.cc"

#undef BRAVE_ANALYSERNODE_CONSTRUCTOR
