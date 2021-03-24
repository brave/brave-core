/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <random>

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/plugin_data.h"
#include "third_party/blink/renderer/modules/plugins/dom_plugin.h"
#include "third_party/blink/renderer/modules/plugins/dom_plugin_array.h"
#include "third_party/blink/renderer/platform/heap/heap.h"

using blink::DOMPlugin;
using blink::DOMPluginArray;
using blink::HeapVector;
using blink::LocalFrame;
using blink::MakeGarbageCollected;
using blink::Member;
using blink::MimeClassInfo;
using blink::PluginInfo;
using WTF::String;

namespace brave {

void FarblePlugins(DOMPluginArray* owner,
                   HeapVector<Member<DOMPlugin>>* dom_plugins) {
  if (!owner->DomWindow())
    return;
  LocalFrame* frame = owner->DomWindow()->GetFrame();
  if (!frame || !frame->GetContentSettingsClient())
    return;
  switch (frame->GetContentSettingsClient()->GetBraveFarblingLevel()) {
    case BraveFarblingLevel::OFF: {
      break;
    }
    case BraveFarblingLevel::BALANCED:
    case BraveFarblingLevel::MAXIMUM: {
      // Both "balanced" and "maximum" behaviors will return a list with 2 fake
      // plugins only since Chromium 90, which changed the navigator.plugins
      // array to always be an empty one unless features::kNavigatorPluginsEmpty
      // is disabled with --disable-features (see https://crrev.com/c/2629990),
      // which is a scenario not supported by Brave.
      DCHECK(dom_plugins->IsEmpty());

      // Add fake plugin #1.
      auto* fake_plugin_info_1 = MakeGarbageCollected<PluginInfo>(
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_1_NAME", 8),
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_1_FILENAME", 16),
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_1_DESCRIPTION", 32),
          0, false);
      auto* fake_mime_info_1 = MakeGarbageCollected<MimeClassInfo>(
          "",
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("MIME_1_DESCRIPTION", 32),
          *fake_plugin_info_1);
      fake_plugin_info_1->AddMimeType(fake_mime_info_1);
      auto* fake_dom_plugin_1 = MakeGarbageCollected<DOMPlugin>(
          frame->DomWindow(), *fake_plugin_info_1);
      dom_plugins->push_back(fake_dom_plugin_1);

      // Add fake plugin #2.
      auto* fake_plugin_info_2 = MakeGarbageCollected<PluginInfo>(
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_2_NAME", 7),
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_2_FILENAME", 15),
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_2_DESCRIPTION", 31),
          0, false);
      auto* fake_mime_info_2 = MakeGarbageCollected<MimeClassInfo>(
          "",
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("MIME_2_DESCRIPTION", 32),
          *fake_plugin_info_2);
      fake_plugin_info_2->AddMimeType(fake_mime_info_2);
      auto* fake_dom_plugin_2 = MakeGarbageCollected<DOMPlugin>(
          frame->DomWindow(), *fake_plugin_info_2);
      dom_plugins->push_back(fake_dom_plugin_2);

      // Shuffle the list of plugins pseudo-randomly, based on the domain key.
      std::mt19937_64 prng = BraveSessionCache::From(*(frame->DomWindow()))
                                 .MakePseudoRandomGenerator();
      std::shuffle(dom_plugins->begin(), dom_plugins->end(), prng);
      break;
    }
    default:
      NOTREACHED();
  }
}

}  // namespace brave

#define BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA \
  GetPluginData()->ResetPluginData();        \
  brave::FarblePlugins(this, &dom_plugins_);

#include "../../../../../../../third_party/blink/renderer/modules/plugins/dom_plugin_array.cc"

#undef BRAVE_DOM_PLUGIN_ARRAY_GET_PLUGIN_DATA
