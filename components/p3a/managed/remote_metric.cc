/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/remote_metric.h"

namespace p3a {

RemoteMetric::RemoteMetric(Delegate* delegate, std::string_view metric_name)
    : delegate_(delegate), metric_name_(metric_name) {}

}  // namespace p3a
