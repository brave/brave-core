// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_origin/brave_origin_service_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "components/policy/policy_constants.h"

BraveOriginPolicyKey const BraveOriginPolicyKeyWalletDisabled =
    base::SysUTF8ToNSString(policy::key::kBraveWalletDisabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyAIChatEnabled =
    base::SysUTF8ToNSString(policy::key::kBraveAIChatEnabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyRewardsDisabled =
    base::SysUTF8ToNSString(policy::key::kBraveRewardsDisabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyTalkDisabled =
    base::SysUTF8ToNSString(policy::key::kBraveTalkDisabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyNewsDisabled =
    base::SysUTF8ToNSString(policy::key::kBraveNewsDisabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyVPNDisabled =
    base::SysUTF8ToNSString(policy::key::kBraveVPNDisabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyP3AEnabled =
    base::SysUTF8ToNSString(policy::key::kBraveP3AEnabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyStatsPingEnabled =
    base::SysUTF8ToNSString(policy::key::kBraveStatsPingEnabled);
