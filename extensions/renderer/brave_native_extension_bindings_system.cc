/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/extensions/renderer/brave_native_extension_bindings_system.h"

#include "base/bind.h"
#include "brave/extensions/renderer/brave_shields_content_setting.h"
#include "extensions/renderer/ipc_message_sender.h"

namespace extensions {

BraveNativeExtensionBindingsSystem::BraveNativeExtensionBindingsSystem(
    std::unique_ptr<IPCMessageSender> ipc_message_sender)
    : NativeExtensionBindingsSystem(std::move(ipc_message_sender)) {
  api_system()->RegisterCustomType("braveShields.ContentSetting",
                                   base::Bind(&BraveShieldsContentSetting::Create));
}

}  // namespace extensions
