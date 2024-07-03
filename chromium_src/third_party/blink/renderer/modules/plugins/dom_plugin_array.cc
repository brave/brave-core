/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/plugins/dom_plugin_array.h"
#include "base/compiler_specific.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/abseil-cpp/absl/random/random.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/plugin_data.h"
#include "third_party/blink/renderer/modules/plugins/dom_plugin.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

using blink::DOMPlugin;
using blink::DOMPluginArray;
using blink::HeapVector;
using blink::LocalFrame;
using blink::MakeGarbageCollected;
using blink::Member;
using blink::MimeClassInfo;
using blink::PluginData;
using blink::PluginInfo;
using WTF::String;
using WTF::StringBuilder;

namespace brave {

String PluginReplacementName(FarblingPRNG* prng) {
  std::vector<String> chrome{"Chrome ", "Chromium ",   "Brave ",
                             "Web ",    "Browser ",    "OpenSource ",
                             "Online ", "JavaScript ", ""};
  std::vector<String> pdf{"PDF ",
                          "Portable Document Format ",
                          "portable-document-format ",
                          "document ",
                          "doc ",
                          "PDF and PS ",
                          "com.adobe.pdf "};
  std::vector<String> viewer{"Viewer",  "Renderer", "Display",   "Plugin",
                             "plug-in", "plug in",  "extension", ""};
  StringBuilder result;
  result.Append(chrome[(*prng)() % chrome.size()]);
  result.Append(pdf[(*prng)() % pdf.size()]);
  result.Append(viewer[(*prng)() % viewer.size()]);
  return result.ToString();
}

void FarblePlugins(DOMPluginArray* owner,
                   PluginData* data,
                   HeapVector<Member<DOMPlugin>>* dom_plugins) {
  // |owner| is guaranteed to be non-null here.
  // |owner->DomWindow()| might be null but function can handle it.
  switch (brave::GetBraveFarblingLevelFor(owner->DomWindow(),
                                          BraveFarblingLevel::OFF)) {
    case BraveFarblingLevel::OFF: {
      break;
    }
    case BraveFarblingLevel::MAXIMUM: {
      dom_plugins->clear();
      // "Maximum" behavior is clear existing plugins + "balanced" behavior,
      // so fall through here.
      [[fallthrough]];
    }
    case BraveFarblingLevel::BALANCED: {
      LocalFrame* frame = owner->DomWindow()->GetFrame();
      FarblingPRNG prng = BraveSessionCache::From(*(frame->DomWindow()))
                              .MakePseudoRandomGenerator();
      // The item() method will populate plugin info if any item of
      // |dom_plugins_| is null, but when it tries, it assumes the
      // length of |dom_plugins_| == the length of the underlying
      // GetPluginData()->Plugins(). Once we add our fake plugins, that
      // assumption will break and the item() method will crash with an
      // out-of-bounds array access. Rather than patch the item() method, we
      // ensure that the cache is fully populated now while the assumptions
      // still hold, so the problematic code is never executed later.
      for (unsigned index = 0; index < dom_plugins->size(); index++) {
        DCHECK_EQ(dom_plugins->size(), data->Plugins().size());
        auto plugin = data->Plugins()[index];
        String name = plugin->Name();
        // Built-in plugins get their names and descriptions farbled as well.
        if ((name == "Chrome PDF Plugin") || (name == "Chrome PDF Viewer")) {
          plugin->SetName(PluginReplacementName(&prng));
          plugin->SetFilename(
              BraveSessionCache::From(*(frame->DomWindow()))
                  .GenerateRandomString(plugin->Filename().Ascii(), 32));
        }
        (*dom_plugins)[index] =
            MakeGarbageCollected<DOMPlugin>(frame->DomWindow(), *plugin);
      }
      // Add fake plugin #1.
      auto* fake_plugin_info_1 = MakeGarbageCollected<PluginInfo>(
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_1_NAME", 8),
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_1_FILENAME", 16),
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_1_DESCRIPTION", 32),
          blink::Color::FromRGBA32(0) /* background_color */, false);
      Vector<String> fake_plugin_extensions_1{
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_1_EXTENSION", 3)};
      auto* fake_mime_info_1 = MakeGarbageCollected<MimeClassInfo>(
          "",
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("MIME_1_DESCRIPTION", 32),
          *fake_plugin_info_1, fake_plugin_extensions_1);
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
          blink::Color::FromRGBA32(0) /* background_color */, false);
      Vector<String> fake_plugin_extensions_2{
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("PLUGIN_2_EXTENSION", 3)};
      auto* fake_mime_info_2 = MakeGarbageCollected<MimeClassInfo>(
          "",
          BraveSessionCache::From(*(frame->DomWindow()))
              .GenerateRandomString("MIME_2_DESCRIPTION", 32),
          *fake_plugin_info_2, fake_plugin_extensions_2);
      fake_plugin_info_2->AddMimeType(fake_mime_info_2);
      auto* fake_dom_plugin_2 = MakeGarbageCollected<DOMPlugin>(
          frame->DomWindow(), *fake_plugin_info_2);
      dom_plugins->push_back(fake_dom_plugin_2);
      // Shuffle the list of plugins pseudo-randomly, based on the domain key.
      std::shuffle(dom_plugins->begin(), dom_plugins->end(), prng);
      break;
    }
    default:
      NOTREACHED_IN_MIGRATION();
  }
}

}  // namespace brave

#define BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__RESET_PLUGIN_DATA \
  if (PluginData* data = GetPluginData())                       \
    data->ResetPluginData();

#define BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__FARBLE_PLUGIN_DATA \
  brave::FarblePlugins(this, data, &dom_plugins_);

#include "src/third_party/blink/renderer/modules/plugins/dom_plugin_array.cc"

#undef BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__FARBLE_PLUGIN_DATA
#undef BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__RESET_PLUGIN_DATA
