// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/brave_domains/service_domains.h"

#include "base/command_line.h"
#include "base/strings/strcat.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/brave_domains/service_domains.h"

namespace {
constexpr char kBraveServicesSwitchValueDev[] = "dev";
constexpr char kBraveServicesSwitchValueStaging[] = "staging";
constexpr char kBraveServicesEnvironmentSwitch[] = "brave-services-env";
}  // namespace

BraveServicesEnvironmentIOS const BraveServicesEnvironmentIOSDevelopment =
    static_cast<NSInteger>(brave_domains::ServicesEnvironment::DEV);
BraveServicesEnvironmentIOS const BraveServicesEnvironmentIOSStaging =
    static_cast<NSInteger>(brave_domains::ServicesEnvironment::STAGING);
BraveServicesEnvironmentIOS const BraveServicesEnvironmentIOSProduction =
    static_cast<NSInteger>(brave_domains::ServicesEnvironment::PROD);

@implementation BraveDomains
+ (BraveServicesEnvironmentIOS)environment {
  return [BraveDomains environmentWithPrefix:@""];
}

+ (BraveServicesEnvironmentIOS)environmentWithPrefix:(NSString*)prefix {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  DCHECK(command_line);

  if (command_line) {
    std::string env_from_switch =
        command_line->GetSwitchValueASCII(kBraveServicesEnvironmentSwitch);

    if ([prefix length] > 0) {
      env_from_switch = command_line->GetSwitchValueASCII(
          base::StrCat({"env-", base::SysNSStringToUTF8(prefix)}));
    }

    if (env_from_switch == kBraveServicesSwitchValueDev) {
      return BraveServicesEnvironmentIOSDevelopment;
    }

    if (env_from_switch == kBraveServicesSwitchValueStaging) {
      return BraveServicesEnvironmentIOSStaging;
    }
  }
  return BraveServicesEnvironmentIOSProduction;
}

+ (NSString*)serviceDomainWithPrefix:(NSString*)prefix {
  return base::SysUTF8ToNSString(
      brave_domains::GetServicesDomain(base::SysNSStringToUTF8(prefix)));
}

+ (NSString*)serviceDomainWithPrefix:(NSString*)prefix
                         environment:(BraveServicesEnvironmentIOS)environment {
  return base::SysUTF8ToNSString(brave_domains::GetServicesDomain(
      base::SysNSStringToUTF8(prefix),
      static_cast<brave_domains::ServicesEnvironment>(environment)));
}
@end
