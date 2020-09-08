/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl_helper.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

using blink::LocalFrame;
using WTF::String;
using WTF::StringBuilder;

namespace brave {

const int kFarbledUserAgentMaxExtraSpaces = 5;

String FarbledUserAgent(LocalFrame* frame, std::mt19937_64 prng) {
  StringBuilder result;
  result.Append(frame->Loader().UserAgent());
  int extra = prng() % kFarbledUserAgentMaxExtraSpaces;
  for (int i = 0; i < extra; i++)
    result.Append(" ");
  return result.ToString();
}

}  // namespace brave

#define BRAVE_NAVIGATOR_USERAGENT                                    \
  if (!AllowFingerprinting(GetFrame()))                              \
    return brave::FarbledUserAgent(                                  \
        GetFrame(),                                                  \
        brave::BraveSessionCache::From(*(GetFrame()->GetDocument())) \
            .MakePseudoRandomGenerator());

#include "../../../../../../../third_party/blink/renderer/core/frame/navigator.cc"

#undef BRAVE_NAVIGATOR_USERAGENT
