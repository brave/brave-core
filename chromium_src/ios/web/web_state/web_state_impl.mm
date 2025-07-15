// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/web/web_state/web_state_impl.h"

#include "ios/web/public/navigation/web_state_policy_decider.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"
#include "ios/web/web_state/web_state_impl_realized_web_state.h"
#include "ios/web/web_state/web_state_impl_serialized_data.h"
#include "ios/web/webui/web_ui_ios_controller_factory_registry.h"
#include "ios/web/webui/web_ui_ios_impl.h"
#include "url/gurl.h"
#include "url/origin.h"

#define TearDown \
  TearDown();    \
  this->TearDownBraveWebUI
#include <ios/web/web_state/web_state_impl.mm>
#undef TearDown

namespace web {

std::unique_ptr<WebUIIOS> CreateWebUIIOS(const GURL& url, WebStateImpl* owner) {
  WebUIIOSControllerFactory* factory =
      WebUIIOSControllerFactoryRegistry::GetInstance();
  if (!factory) {
    return nullptr;
  }
  std::unique_ptr<WebUIIOS> web_ui = std::make_unique<WebUIIOSImpl>(owner);
  auto controller = factory->CreateWebUIIOSControllerForURL(web_ui.get(), url);
  if (!controller) {
    return nullptr;
  }

  web_ui->SetController(std::move(controller));
  return web_ui;
}

void WebStateImpl::TearDownBraveWebUI() {
  brave_web_uis_.clear();
}

void WebStateImpl::CreateBraveWebUI(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(RealizedState());

  if (HasBraveWebUI()) {
    for (auto& web_ui : brave_web_uis_) {
      if (web_ui->GetController()->GetHost() == url.host()) {
        return;
      }
    }

    // ClearWebUI();
  }

  auto web_ui = CreateWebUIIOS(url, this);
  if (web_ui) {
    brave_web_uis_.emplace_back(std::move(web_ui));
  }
}

void WebStateImpl::ClearBraveWebUI() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(RealizedState());

  brave_web_uis_.clear();
}

bool WebStateImpl::HasBraveWebUI() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return !brave_web_uis_.empty();
}

void WebStateImpl::HandleBraveWebUIMessage(const GURL& source_url,
                                           std::string_view message,
                                           const base::Value::List& args) {
  auto origin = url::Origin::Create(source_url);
  if (origin.opaque()) {
    return;
  }

  for (auto& web_ui : brave_web_uis_) {
    if (web_ui->GetController()->GetHost() == origin.host()) {
      web_ui->ProcessWebUIIOSMessage(source_url, message, args);
    }
  }
}

web::WebUIIOS* WebStateImpl::GetMainWebUI() const {
  if (!brave_web_uis_.empty()) {
    return brave_web_uis_.at(0).get();
  }
  return nullptr;
}

}  // namespace web
