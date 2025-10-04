/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/plugins/dom_plugin_array.h"

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/plugin_data.h"
#include "third_party/blink/renderer/modules/plugins/dom_plugin.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"
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
using blink::StrCat;
using blink::String;
using blink::StringBuilder;

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
  return StrCat({chrome[(*prng)() % chrome.size()], pdf[(*prng)() % pdf.size()],
                 viewer[(*prng)() % viewer.size()]});
}

// General helper to add a mime type to a plugin info, with a single extension
MimeClassInfo* AddMimeTypeToPluginInfo(PluginInfo* plugin_info,
                                       const String& mime_type,
                                       const String& description,
                                       const char* extension) {
  blink::Vector<String> extensions{String(extension)};
  auto* mime_info = MakeGarbageCollected<MimeClassInfo>(
      mime_type, description, *plugin_info, extensions);
  plugin_info->AddMimeType(mime_info);
  return mime_info;
}

// Helper to create a fake plugin with multiple mime types
DOMPlugin* BraveMakeFakePlugin(blink::LocalDOMWindow* window,
                               const String& name,
                               const String& filename,
                               const String& description,
                               const std::vector<const char*>& mime_types,
                               const String& mime_description,
                               const char* plugin_extension) {
  auto* plugin_info = MakeGarbageCollected<PluginInfo>(
      name, filename, description, blink::Color::FromRGBA32(0), false);
  for (const char* mime_type : mime_types) {
    AddMimeTypeToPluginInfo(plugin_info, mime_type, mime_description,
                            plugin_extension);
  }
  return MakeGarbageCollected<DOMPlugin>(window, *plugin_info);
}

void FarblePlugins(blink::LocalDOMWindow* window,
                   HeapVector<Member<DOMPlugin>>& dom_plugins) {
  // |window| might be null but function can handle it.
  auto farbling_level = brave::GetBraveFarblingLevelFor(
      window, ContentSettingsType::BRAVE_WEBCOMPAT_PLUGINS,
      BraveFarblingLevel::OFF);
  switch (farbling_level) {
    case BraveFarblingLevel::OFF: {
      return;
    }
    case BraveFarblingLevel::MAXIMUM: {
      dom_plugins.clear();
      // "Maximum" behavior is clear existing plugins + "balanced" behavior,
      // so fall through here.
      [[fallthrough]];
    }
    case BraveFarblingLevel::BALANCED: {
      FarblingPRNG prng =
          BraveSessionCache::From(*(window)).MakePseudoRandomGenerator();
      for (unsigned index = 0; index < dom_plugins.size(); index++) {
        auto plugin = dom_plugins[index];
        String name = plugin->name();
        String description = plugin->description();
        // Built-in plugins get their names and descriptions farbled as well.
        if ((name == "Chrome PDF Plugin") || (name == "Chrome PDF Viewer")) {
          std::vector<const char*> mime_types{"application/pdf", "text/pdf"};
          dom_plugins[index] = BraveMakeFakePlugin(
              window, PluginReplacementName(&prng),
              BraveSessionCache::From(*window).GenerateRandomString(
                  plugin->filename().Ascii(), 32),
              description, mime_types, description, "pdf");
        }
      }
      // Add fake plugin #1.
      dom_plugins.push_back(BraveMakeFakePlugin(
          window,
          BraveSessionCache::From(*window).GenerateRandomString("PLUGIN_1_NAME",
                                                                8),
          BraveSessionCache::From(*window).GenerateRandomString(
              "PLUGIN_1_FILENAME", 16),
          BraveSessionCache::From(*window).GenerateRandomString(
              "PLUGIN_1_DESCRIPTION", 32),
          std::vector<const char*>{"", ""},
          BraveSessionCache::From(*window).GenerateRandomString(
              "MIME_1_DESCRIPTION", 32),
          BraveSessionCache::From(*window)
              .GenerateRandomString("PLUGIN_1_EXTENSION", 3)
              .Ascii()
              .data()));
      // Add fake plugin #2.
      dom_plugins.push_back(BraveMakeFakePlugin(
          window,
          BraveSessionCache::From(*window).GenerateRandomString("PLUGIN_2_NAME",
                                                                7),
          BraveSessionCache::From(*window).GenerateRandomString(
              "PLUGIN_2_FILENAME", 15),
          BraveSessionCache::From(*window).GenerateRandomString(
              "PLUGIN_2_DESCRIPTION", 31),
          std::vector<const char*>{"", ""},
          BraveSessionCache::From(*window).GenerateRandomString(
              "MIME_2_DESCRIPTION", 32),
          BraveSessionCache::From(*window)
              .GenerateRandomString("PLUGIN_2_EXTENSION", 3)
              .Ascii()
              .data()));
      // Shuffle the list of plugins pseudo-randomly, based on the domain key.
      std::shuffle(dom_plugins.begin(), dom_plugins.end(), prng);
      return;
    }
  }
  NOTREACHED() << "Unexpected value for farbling_level: "
               << base::to_underlying(farbling_level);
}

}  // namespace brave

#define BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__FARBLE_PLUGIN_DATA \
  brave::FarblePlugins(window, dom_plugins_);

#include <third_party/blink/renderer/modules/plugins/dom_plugin_array.cc>

#undef BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__FARBLE_PLUGIN_DATA
