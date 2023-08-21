/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_

#include "brave/components/widevine/static_buildflags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#ifdef RegisterWidevineCdmComponent
#pragma push_macro("RegisterWidevineCdmComponent")
#undef RegisterWidevineCdmComponent
#define RegisterWidevineCdmComponent_WasDefined
#endif

namespace component_updater {

class ComponentUpdateService;

void RegisterWidevineCdmComponent(
    ComponentUpdateService* cus,
#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
#endif
    base::OnceCallback<void()> callback = base::DoNothing());

}  // namespace component_updater

#ifdef RegisterWidevineCdmComponent_WasDefined
#pragma pop_macro("RegisterWidevineCdmComponent")
#undef RegisterWidevineCdmComponent_WasDefined
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_COMPONENT_UPDATER_WIDEVINE_CDM_COMPONENT_INSTALLER_H_
