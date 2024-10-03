// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/skus/skus_internals_ui.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/resources/grit/skus_internals_generated_map.h"
#include "brave/ios/browser/skus/skus_service_factory.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/ui/util/pasteboard_util.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/url_data_source_ios.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_data_source.h"
#include "ios/web/public/webui/web_ui_ios_message_handler.h"
#include "ui/base/clipboard/clipboard_ios.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/webui/resource_path.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

web::WebUIIOSDataSource* CreateAndAddWebUIDataSource(
    web::WebUIIOS* web_ui,
    const std::string& name,
    const webui::ResourcePath* resource_map,
    std::size_t resource_map_size,
    int html_resource_id) {
  web::WebUIIOSDataSource* source = web::WebUIIOSDataSource::Create(name);
  web::WebUIIOSDataSource::Add(ChromeBrowserState::FromWebUIIOS(web_ui),
                               source);
  source->UseStringsJs();

  // Add required resources.
  source->AddResourcePaths(base::make_span(resource_map, resource_map_size));
  source->SetDefaultResource(html_resource_id);
  return source;
}

UIViewController* GetParentControllerFromView(UIView* view) {
  UIResponder* nextResponder = [view nextResponder];
  if ([nextResponder isKindOfClass:[UIViewController class]]) {
    return static_cast<UIViewController*>(nextResponder);
  }

  if ([nextResponder isKindOfClass:[UIView class]]) {
    return GetParentControllerFromView(static_cast<UIView*>(nextResponder));
  }

  return nil;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// SkuesInternalsUI
//
///////////////////////////////////////////////////////////////////////////////

SkusInternalsUI::SkusInternalsUI(web::WebUIIOS* web_ui, const GURL& url)
    : web::WebUIIOSController(web_ui, url.host()),
      local_state_(GetApplicationContext()->GetLocalState()) {
  // Set up the brave://skus-internals/ source.
  CreateAndAddWebUIDataSource(web_ui, url.host(), kSkusInternalsGenerated,
                              kSkusInternalsGeneratedSize,
                              IDR_SKUS_INTERNALS_HTML);

  ChromeBrowserState* browser_state = ChromeBrowserState::FromWebUIIOS(web_ui);
  skus_service_getter_ = base::BindRepeating(
      [](ChromeBrowserState* browser_state) {
        return skus::SkusServiceFactory::GetForBrowserState(browser_state);
      },
      browser_state);

  // Bind Mojom Interface
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&SkusInternalsUI::BindInterface,
                          base::Unretained(this)));
}

SkusInternalsUI::~SkusInternalsUI() {}

void SkusInternalsUI::BindInterface(
    mojo::PendingReceiver<skus::mojom::SkusInternals> pending_receiver) {
  if (skus_internals_receiver_.is_bound()) {
    skus_internals_receiver_.reset();
  }

  skus_internals_receiver_.Bind(std::move(pending_receiver));
}

void SkusInternalsUI::GetEventLog(GetEventLogCallback callback) {
  // TODO(simonhong): Ask log to SkusService
  NOTIMPLEMENTED_LOG_ONCE();
}

void SkusInternalsUI::GetSkusState(GetSkusStateCallback callback) {
  std::move(callback).Run(GetSkusStateAsString());
}

void SkusInternalsUI::GetVpnState(GetVpnStateCallback callback) {
  base::Value::Dict dict;
  dict.Set("Order", GetOrderInfo("vpn."));
  std::string result;
  base::JSONWriter::Write(dict, &result);
  std::move(callback).Run(result);
}

void SkusInternalsUI::GetLeoState(GetLeoStateCallback callback) {
  base::Value::Dict dict;
  dict.Set("Order", GetOrderInfo("leo."));
  std::string result;
  base::JSONWriter::Write(dict, &result);
  std::move(callback).Run(result);
}

base::Value::Dict SkusInternalsUI::GetOrderInfo(
    const std::string& location) const {
  base::Value::Dict dict;

  const auto& skus_state = local_state_->GetDict(skus::prefs::kSkusState);
  for (const auto kv : skus_state) {
    if (!base::StartsWith(kv.first, "skus:")) {
      continue;
    }

    // Convert to Value as it's stored as string in local state.
    auto json_value = base::JSONReader::Read(kv.second.GetString());
    if (!json_value) {
      continue;
    }

    const auto* skus = json_value->GetIfDict();
    if (!skus) {
      continue;
    }

    const auto* orders = skus->FindDict("orders");
    if (!orders) {
      continue;
    }

    base::Value::Dict order_dict_output;
    for (const auto order : *orders) {
      const auto* order_dict = order.second.GetIfDict();
      if (!order_dict) {
        continue;
      }

      if (auto* order_location = order_dict->FindString("location")) {
        if (!base::StartsWith(*order_location, location)) {
          continue;
        }
        order_dict_output.Set("location", *order_location);
      }

      if (auto* id = order_dict->FindString("id")) {
        order_dict_output.Set("id", *id);
      }
      if (auto* expires_at = order_dict->FindString("expires_at")) {
        order_dict_output.Set("expires_at", *expires_at);
      }
    }

    // Set output with env like {skus:production: {...}}.
    dict.Set(kv.first, std::move(order_dict_output));
  }
  return dict;
}

void SkusInternalsUI::ResetSkusState() {
  local_state_->ClearPref(skus::prefs::kSkusState);
}

void SkusInternalsUI::CopySkusStateToClipboard() {
  StoreTextInPasteboard(base::SysUTF8ToNSString(GetSkusStateAsString()));
}

void SkusInternalsUI::DownloadSkusState() {
  UIViewController* controller =
      GetParentControllerFromView(web_ui()->GetWebState()->GetView());
  if (controller) {
    NSString* skus_state = base::SysUTF8ToNSString(GetSkusStateAsString());

    UIActivityViewController* activityController =
        [[UIActivityViewController alloc] initWithActivityItems:@[ skus_state ]
                                          applicationActivities:nil];

    [controller presentViewController:activityController
                             animated:true
                           completion:nil];
  }
}

std::string SkusInternalsUI::GetSkusStateAsString() const {
  const auto& skus_state = local_state_->GetDict(skus::prefs::kSkusState);
  base::Value::Dict dict;

  for (const auto kv : skus_state) {
    // Only shows "skus:xx" kv in webui.
    if (!base::StartsWith(kv.first, "skus:")) {
      continue;
    }

    if (auto value = base::JSONReader::Read(kv.second.GetString()); value) {
      dict.Set(kv.first, std::move(*value));
    }
  }

  std::string result;
  base::JSONWriter::Write(dict, &result);
  return result;
}

void SkusInternalsUI::EnsureMojoConnected() {
  if (!skus_service_) {
    auto pending = skus_service_getter_.Run();
    skus_service_.Bind(std::move(pending));
  }
  DCHECK(skus_service_);
  skus_service_.set_disconnect_handler(base::BindOnce(
      &SkusInternalsUI::OnMojoConnectionError, base::Unretained(this)));
}

void SkusInternalsUI::OnMojoConnectionError() {
  skus_service_.reset();
  EnsureMojoConnected();
}

void SkusInternalsUI::CreateOrderFromReceipt(
    const std::string& domain,
    const std::string& receipt,
    CreateOrderFromReceiptCallback callback) {
  EnsureMojoConnected();

  skus_service_->CreateOrderFromReceipt(domain, receipt,
                                        base::BindOnce(std::move(callback)));
}
