/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "components/update_client/update_client.h"

namespace {
const base::Feature kEnforceCRX3PublisherProof{
    "EnforceCRX3PublisherProof", base::FEATURE_ENABLED_BY_DEFAULT};

crx_file::VerifierFormat GetVerifierFormat() {
  if (base::FeatureList::IsEnabled(kEnforceCRX3PublisherProof))
    return crx_file::VerifierFormat::CRX3_WITH_PUBLISHER_PROOF;

  return crx_file::VerifierFormat::CRX3;
}
}  // namespace

#define crx_format_requirement                         \
  crx_format_requirement = GetVerifierFormat();        \
  [[maybe_unused]] crx_file::VerifierFormat _temp_var; \
  _temp_var
#include "src/components/component_updater/component_updater_service.cc"
#undef crx_format_requirement
