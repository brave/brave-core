/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BASE_BUILDFLAG_CONFIG_H_
#define BRAVE_BASE_BUILDFLAG_CONFIG_H_

#include <string>
#include <string_view>

#include "build/buildflag.h"

#define BUILDFLAG_CONFIG(flag) \
  brave::BuildflagConfig(#flag, BUILDFLAG(flag)).Get()

#define SCOPED_BUILDFLAG_CONFIG_OVERRIDE(flag, value)                        \
  brave::ScopedBuildflagConfigOverride scoped_buildflag_config_##flag(#flag, \
                                                                      value);

namespace brave {

class BuildflagConfig {
 public:
  BuildflagConfig(std::string_view name, std::string_view value);

  virtual std::string Get() const;

 protected:
  std::string name_;
  std::string value_;
};

class ScopedBuildflagConfigOverride : BuildflagConfig {
 public:
  ScopedBuildflagConfigOverride(std::string_view name, std::string_view value);
  ~ScopedBuildflagConfigOverride();

  std::string Get() const override;
};

}  // namespace brave

#endif  // BRAVE_BASE_BUILDFLAG_CONFIG_H_
