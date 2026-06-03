// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/test/ad_block_unit_test_helper.h"

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

namespace brave_shields {

void SetupAdBlockServiceForTesting(AdBlockService* ad_block_service) {
  // Post the empty-catalog load instead of running it synchronously. If the
  // gates are initialized synchronously, a test that registers a filter
  // provider immediately after this call ends up with two in-flight engine
  // builds (an empty one from the gate and a rule-containing one from the
  // provider), and the service-observer Wait can race and observe the empty
  // build. Posting defers the gate init until after the caller returns, so
  // the test's provider is already registered when the gate fires — the
  // manager then issues a single engine build containing the test's rules.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](AdBlockComponentServiceManager* m) {
            m->SetFilterListCatalog({});
          },
          base::Unretained(ad_block_service->component_service_manager())));
}

}  // namespace brave_shields
