/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated_map.h"
#include "content/public/renderer/render_frame.h"
#include "ui/base/resource/resource_bundle.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace {

static base::NoDestructor<std::string> g_provider_script("");

std::string LoadDataResource(const int id) {
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  if (resource_bundle.IsGzipped(id)) {
    return resource_bundle.LoadDataResourceString(id);
  }

  return resource_bundle.GetRawDataResource(id).as_string();
}

}  // namespace

namespace brave_wallet {

BraveWalletJSHandler::BraveWalletJSHandler(
    content::RenderFrame* render_frame)
    : render_frame_(render_frame) {
  if (g_provider_script->empty()) {
    *g_provider_script = LoadDataResource(kBraveWalletScriptGenerated[0].value);
  }
}

BraveWalletJSHandler::~BraveWalletJSHandler() = default;

void BraveWalletJSHandler::InjectScript() {
  blink::WebLocalFrame* web_frame = render_frame_->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(blink::WebString::FromUTF8(*g_provider_script));
}

}  // namespace brave_wallet
