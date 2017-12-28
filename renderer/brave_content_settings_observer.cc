/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_settings_observer.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/render_messages.h"
#include "content/public/renderer/render_frame.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/WebKit/public/platform/WebURL.h"

BraveContentSettingsObserver::BraveContentSettingsObserver(
    content::RenderFrame* render_frame,
    extensions::Dispatcher* extension_dispatcher,
    bool should_whitelist,
    service_manager::BinderRegistry* registry)
    : ContentSettingsObserver(render_frame, extension_dispatcher,
          should_whitelist, registry) {
}

BraveContentSettingsObserver::~BraveContentSettingsObserver() {
}

void BraveContentSettingsObserver::BraveSpecificDidBlockJavaScript(
    const base::string16& details) {
  Send(new BraveViewHostMsg_JavaScriptBlocked(routing_id(), details));
}

bool BraveContentSettingsObserver::AllowScriptFromSource(
    bool enabled_per_settings,
    const blink::WebURL& script_url) {
  bool allow = ContentSettingsObserver::AllowScriptFromSource(
      enabled_per_settings, script_url);
  if (!allow) {
    const GURL secondary_url(script_url);
    BraveSpecificDidBlockJavaScript(base::UTF8ToUTF16(secondary_url.spec()));
  }
  return allow;
}

