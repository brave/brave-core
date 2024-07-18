/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/reporting_observer.h"

#define QueueReport QueueReport_Unused
#include "src/third_party/blink/renderer/core/frame/reporting_observer.cc"
#undef QueueReport

namespace blink {

void ReportingObserver::QueueReport(Report* report) {
  return;
}

}  // namespace blink
