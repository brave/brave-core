/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/default_locale_util.h"

#include <optional>
#include <string>

#include "base/environment.h"

namespace brave_l10n {

namespace {

constexpr char kEnvVarLcAll[] = "LC_ALL";
constexpr char kEnvVarLang[] = "LANG";

}  // namespace

std::optional<std::string> MaybeGetDefaultLocaleString() {
  std::unique_ptr<base::Environment> env = base::Environment::Create();

  std::optional<std::string> language = env->GetVar(kEnvVarLcAll);
  if (!language || language->empty()) {
    language = env->GetVar(kEnvVarLang);
    if (!language || language->empty()) {
      return std::nullopt;
    }
  }

  return language;
}

}  // namespace brave_l10n
