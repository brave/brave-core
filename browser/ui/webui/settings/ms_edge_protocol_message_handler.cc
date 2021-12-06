// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/ms_edge_protocol_message_handler.h"

#include <shlobj.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "base/win/windows_version.h"
#include "brave/browser/default_protocol_handler_utils_win.h"
#include "chrome/installer/util/shell_util.h"

using protocol_handler_utils::IsDefaultProtocolHandlerFor;
using protocol_handler_utils::SetDefaultProtocolHandlerFor;

namespace {

constexpr wchar_t kMSEdgeProtocol[] = L"microsoft-edge";
constexpr wchar_t kMSEdgeProtocolRegKey[] =
    L"SOFTWARE\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\micro"
    L"soft-edge";

}  // namespace

// static
bool MSEdgeProtocolMessageHandler::CanSetDefaultMSEdgeProtocolHandler() {
  base::win::OSInfo* os_info = base::win::OSInfo::GetInstance();
  const auto& version_number = os_info->version_number();
  // MS will not allow setting 3p application as a default microsoft-edge
  // handler. See
  // https://www.ctrl.blog/entry/microsoft-edge-protocol-competition.html
  // Hope this constraint disappeared!
  if (version_number.major <= 10)
    return true;
  return version_number.build < 22494;
}

MSEdgeProtocolMessageHandler::MSEdgeProtocolMessageHandler()
    : user_choice_key_(HKEY_CURRENT_USER, kMSEdgeProtocolRegKey, KEY_NOTIFY) {
  DCHECK(CanSetDefaultMSEdgeProtocolHandler());
  StartWatching();
}

MSEdgeProtocolMessageHandler::~MSEdgeProtocolMessageHandler() = default;

void MSEdgeProtocolMessageHandler::StartWatching() {
  if (user_choice_key_.Valid()) {
    user_choice_key_.StartWatching(
        base::BindOnce(&MSEdgeProtocolMessageHandler::OnRegValChanged,
                       base::Unretained(this)));
  }
}

void MSEdgeProtocolMessageHandler::OnRegValChanged() {
  CheckMSEdgeProtocolDefaultHandlerState();
  StartWatching();
}

void MSEdgeProtocolMessageHandler::CheckMSEdgeProtocolDefaultHandlerState() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&IsDefaultProtocolHandlerFor, kMSEdgeProtocol),
      base::BindOnce(&MSEdgeProtocolMessageHandler::OnIsDefaultProtocolHandler,
                     weak_factory_.GetWeakPtr()));
}

void MSEdgeProtocolMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "checkDefaultMSEdgeProtocolHandlerState",
      base::BindRepeating(&MSEdgeProtocolMessageHandler::
                              HandleCheckDefaultMSEdgeProtocolHandlerState,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "setAsDefaultMSEdgeProtocolHandler",
      base::BindRepeating(&MSEdgeProtocolMessageHandler::
                              HandleSetAsDefaultMSEdgeProtocolHandler,
                          base::Unretained(this)));
}

void MSEdgeProtocolMessageHandler::HandleCheckDefaultMSEdgeProtocolHandlerState(
    base::Value::ConstListView args) {
  AllowJavascript();
  CheckMSEdgeProtocolDefaultHandlerState();
}

void MSEdgeProtocolMessageHandler::HandleSetAsDefaultMSEdgeProtocolHandler(
    base::Value::ConstListView args) {
  AllowJavascript();

  // Test purpose switch to use system ui.
  constexpr char kUseSystemUIForMSEdgeProtocol[] = "use-system-ui-for-ms-edge";
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kUseSystemUIForMSEdgeProtocol)) {
    LaunchSystemDialog();
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindOnce(&SetDefaultProtocolHandlerFor, kMSEdgeProtocol),
      base::BindOnce(&MSEdgeProtocolMessageHandler::OnSetDefaultProtocolHandler,
                     weak_factory_.GetWeakPtr()));
}

void MSEdgeProtocolMessageHandler::OnIsDefaultProtocolHandler(bool is_default) {
  if (IsJavascriptAllowed()) {
    FireWebUIListener("notify-ms-edge-protocol-default-handler-status",
                      base::Value(is_default));
  }
}

void MSEdgeProtocolMessageHandler::OnSetDefaultProtocolHandler(bool success) {
  if (!success) {
    LaunchSystemDialog();
    return;
  }

  if (IsJavascriptAllowed()) {
    FireWebUIListener("notify-ms-edge-protocol-default-handler-status",
                      base::Value(success));
  }

  ::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

void MSEdgeProtocolMessageHandler::LaunchSystemDialog() {
  base::FilePath brave_exe;
  if (!base::PathService::Get(base::FILE_EXE, &brave_exe)) {
    LOG(ERROR) << "Failed to get app exe path";
    return;
  }

  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindOnce(
          [](const base::FilePath& brave_exe) {
            ShellUtil::ShowMakeChromeDefaultProtocolClientSystemUI(
                brave_exe, kMSEdgeProtocol);
          },
          brave_exe));
}
