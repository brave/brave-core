/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/registration.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"

#define RegisterComponentsForUpdate RegisterComponentsForUpdate_ChromiumImpl

#include <chrome/browser/component_updater/registration.cc>

#undef RegisterComponentsForUpdate

#include "brave/browser/brave_browser_process.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_component_installer.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "brave/components/p3a/component_installer.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/query_filter/browser/query_filter_component_installer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/component_updater_utils.h"

#if BUILDFLAG(ENABLE_PSST)
#include "brave/components/psst/core/browser/psst_component_installer.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/web_mcp/core/browser/web_mcp_component_installer.h"
#include "third_party/blink/public/common/features.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/component_updater/zxcvbn_data_component_installer.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/local_models_updater.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#endif

namespace component_updater {

void RegisterComponentsForUpdate() {
  RegisterComponentsForUpdate_ChromiumImpl();
  ComponentUpdateService* cus = g_browser_process->component_updater();
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  brave_wallet::WalletDataFilesInstaller::GetInstance()
      .MaybeRegisterWalletDataFilesComponent(cus,
                                             g_browser_process->local_state());
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)
#if BUILDFLAG(ENABLE_PSST)
  psst::RegisterPsstComponent(cus);
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  // Only fetch WebMCP tool scripts when the runtime feature is enabled; the
  // per-tab injector (WebMcpInjector) is gated on the same feature.
  if (base::FeatureList::IsEnabled(blink::features::kWebMCP)) {
    web_mcp::RegisterWebMcpComponent(cus);
  }
#endif
  p3a::MaybeToggleP3AComponent(cus, g_brave_browser_process->p3a_service());
#if BUILDFLAG(IS_ANDROID)
  // Currently behind !BUILDFLAG(IS_ANDROID) in upstream.
  RegisterZxcvbnDataComponent(cus);
#endif  // BUILDFLAG(IS_ANDROID)
  brave_user_agent::RegisterBraveUserAgentComponent(cus);
#if BUILDFLAG(ENABLE_LOCAL_AI)
  local_ai::ManageLocalModelsComponentRegistration(
      cus, g_browser_process->local_state());
  local_ai::RegisterOnDeviceSpeechModelsComponent(cus);
#endif
  RegisterQueryFilterComponent(cus);
}

}  // namespace component_updater
