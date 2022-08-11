/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/metric_log_type.h"

#include "base/notreached.h"

namespace brave {

namespace {
constexpr char kTypicalStr[] = "typical";
constexpr char kExpressStr[] = "express";
}  // namespace

const char* MetricLogTypeToString(MetricLogType log_type) {
  switch (log_type) {
    case MetricLogType::kTypical:
      return kTypicalStr;
    case MetricLogType::kExpress:
      return kExpressStr;
  }
  NOTREACHED();
}

}  // namespace brave
