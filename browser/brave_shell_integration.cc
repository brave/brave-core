/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_shell_integration.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include <shlobj.h>

#include <utility>

#include "brave/browser/brave_shell_integration_win.h"
#include "brave/browser/default_protocol_handler_utils_win.h"
#include "chrome/installer/util/shell_util.h"
#endif

#if BUILDFLAG(IS_MAC)
#include "brave/browser/brave_shell_integration_mac.h"
#endif

namespace shell_integration {

void PinShortcut(Profile* profile,
                 base::OnceCallback<void(bool)> result_callback) {
#if BUILDFLAG(IS_WIN)
  win::PinToTaskbar(profile, std::move(result_callback));
#elif BUILDFLAG(IS_MAC)
  // Mac doesn't support profile specific icon in dock.
  mac::AddIconToDock(std::move(result_callback));
#elif BUILDFLAG(IS_LINUX)
  NOTREACHED_IN_MIGRATION() << "Not supported on linux yet.";
#endif
}

void IsShortcutPinned(base::OnceCallback<void(bool)> result_callback) {
#if BUILDFLAG(IS_WIN)
  win::IsShortcutPinned(std::move(result_callback));
#elif BUILDFLAG(IS_MAC)
  mac::IsIconAddedToDock(std::move(result_callback));
#elif BUILDFLAG(IS_LINUX)
  NOTREACHED_IN_MIGRATION() << "Not supported on linux yet.";
#endif
}

BraveDefaultBrowserWorker::BraveDefaultBrowserWorker() = default;
BraveDefaultBrowserWorker::~BraveDefaultBrowserWorker() = default;

void BraveDefaultBrowserWorker::SetAsDefaultImpl(
    base::OnceClosure on_finished_callback) {
#if BUILDFLAG(IS_WIN)
  if (GetDefaultBrowserSetPermission() != SET_DEFAULT_NOT_ALLOWED) {
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
