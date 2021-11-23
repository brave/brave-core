/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration.h"

#if defined(OS_WIN)
#include <shlobj.h>

#include <utility>

#include "brave/browser/default_protocol_handler_utils_win.h"
#include "chrome/installer/util/shell_util.h"
#endif

namespace shell_integration {

BraveDefaultBrowserWorker::BraveDefaultBrowserWorker() = default;
BraveDefaultBrowserWorker::~BraveDefaultBrowserWorker() = default;

void BraveDefaultBrowserWorker::SetAsDefaultImpl(
    base::OnceClosure on_finished_callback) {
#if defined(OS_WIN)
  if (GetDefaultWebClientSetPermission() != SET_DEFAULT_NOT_ALLOWED) {
    bool success = false;
    const wchar_t* kAssociations[] = {L"https", L"http", L".html", L".htm"};
    for (const wchar_t* association : kAssociations) {
      success =
          protocol_handler_utils::SetDefaultProtocolHandlerFor(association);
      if (!success)
        break;
    }

    if (success) {
      std::move(on_finished_callback).Run();

      // Notify shell to refresh icons
      ::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
      return;
    }
  }
#endif
  DefaultBrowserWorker::SetAsDefaultImpl(std::move(on_finished_callback));
}

}  // namespace shell_integration
