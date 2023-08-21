/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_

#include "brave/components/widevine/static_buildflags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#define RegisterWidevineCdmComponent(cus)                                     \
  RegisterWidevineCdmComponent(                                               \
      cus, scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory, \
      base::OnceCallback<void()> callback = base::DoNothing())
#else
#define RegisterWidevineCdmComponent(cus) \
  RegisterWidevineCdmComponent(           \
      cus, base::OnceCallback<void()> callback = base::DoNothing())
#endif

#include "src/chrome/browser/component_updater/widevine_cdm_component_installer.h"  // IWYU pragma: export

#undef RegisterWidevineCdmComponent

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
