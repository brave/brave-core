// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/skus_internals_ui.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/skus/skus_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/resources/grit/skus_internals_generated_map.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "components/grit/brave_components_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/shell_dialogs/selected_file_info.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

namespace {

void SaveSkusStateToFile(const base::FilePath& path,
                         std::string_view skus_state) {
  base::WriteFile(path, skus_state);
}

}  // namespace

SkusInternalsUI::SkusInternalsUI(content::WebUI* web_ui,
                                 const std::string& name)
    : content::WebUIController(web_ui),
      local_state_(g_browser_process->local_state()) {
  CreateAndAddWebUIDataSource(web_ui, name, kSkusInternalsGenerated,
                              kSkusInternalsGeneratedSize,
                              IDR_SKUS_INTERNALS_HTML);

  auto* web_contents = web_ui->GetWebContents();
  content::BrowserContext* context = web_contents->GetBrowserContext();
  skus_service_getter_ = base::BindRepeating(
      [](content::BrowserContext* context) {
        return skus::SkusServiceFactory::GetForContext(context);
      },
      context);
}

SkusInternalsUI::~SkusInternalsUI() = default;

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
#if BUILDFLAG(ENABLE_BRAVE_VPN)
#if !BUILDFLAG(IS_ANDROID)
  dict.Set("Last connection error", GetLastVPNConnectionError());
#endif
  auto* profile = Profile::FromWebUI(web_ui());
  if (!brave_vpn::IsBraveVPNEnabled(profile->GetPrefs())) {
    dict.Set("Order", base::Value::Dict());
  } else {
    auto order_info = GetOrderInfo("vpn.");
    order_info.Set(
        "env", local_state_->GetString(brave_vpn::prefs::kBraveVPNEnvironment));
    dict.Set("Order", std::move(order_info));
  }
#endif
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
    if (!kv.first.starts_with("skus:")) {
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
        if (!order_location->starts_with(location)) {
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
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // VPN service caches credentials. It should be cleared also
  // when skus state is reset. Otherwise, vpn service is still
  // in purchased state.
  auto* profile = Profile::FromWebUI(web_ui());
  if (brave_vpn::IsBraveVPNEnabled(profile->GetPrefs())) {
    brave_vpn::ClearSubscriberCredential(local_state_);
    if (auto* service =
            brave_vpn::BraveVpnServiceFactory::GetForProfile(profile)) {
      service->ReloadPurchasedState();
    }
  }
#endif

  local_state_->ClearPref(skus::prefs::kSkusState);
}

void SkusInternalsUI::CopySkusStateToClipboard() {
  ui::ScopedClipboardWriter(ui::ClipboardBuffer::kCopyPaste)
      .WriteText(base::UTF8ToUTF16(GetSkusStateAsString()));
}

void SkusInternalsUI::DownloadSkusState() {
  if (select_file_dialog_) {
    return;
  }

  auto* web_contents = web_ui()->GetWebContents();
  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(web_contents));
  if (!select_file_dialog_) {
    return;
  }

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.allowed_paths = ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_SAVEAS_FILE, std::u16string(),
      profile->last_selected_directory().AppendASCII("skus_state.json"),
      &file_types, 0, base::FilePath::StringType(),
      web_contents->GetTopLevelNativeWindow(), nullptr);
}

void SkusInternalsUI::FileSelected(const ui::SelectedFileInfo& file,
                                   int index) {
  auto* web_contents = web_ui()->GetWebContents();
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  profile->set_last_selected_directory(file.path().DirName());
  select_file_dialog_ = nullptr;

  base::ThreadPool::PostTask(FROM_HERE, {base::MayBlock()},
                             base::BindOnce(&SaveSkusStateToFile, file.path(),
                                            GetSkusStateAsString()));
}

void SkusInternalsUI::FileSelectionCanceled() {
  select_file_dialog_ = nullptr;
}

std::string SkusInternalsUI::GetLastVPNConnectionError() const {
  std::string error;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* manager = g_brave_browser_process->brave_vpn_connection_manager();
  CHECK(manager);
  error = manager->GetLastConnectionError();
#endif
  return error;
}

std::string SkusInternalsUI::GetSkusStateAsString() const {
  const auto& skus_state = local_state_->GetDict(skus::prefs::kSkusState);
  base::Value::Dict dict;

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  auto* profile = Profile::FromWebUI(web_ui());
  if (brave_vpn::IsBraveVPNEnabled(profile->GetPrefs())) {
    dict.Set("env",
             local_state_->GetString(brave_vpn::prefs::kBraveVPNEnvironment));
  }
#endif

  for (const auto kv : skus_state) {
    // Only shows "skus:xx" kv in webui.
    if (!kv.first.starts_with("skus:")) {
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

  // TODO(bsclifton): remove once this has been tested.
  LOG(ERROR) << "skus-internals> CreateOrderFromReceipt> domain=" << domain
             << " | receipt=" << receipt;
  return;
}

WEB_UI_CONTROLLER_TYPE_IMPL(SkusInternalsUI)
