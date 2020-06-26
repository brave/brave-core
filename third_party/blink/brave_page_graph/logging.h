/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_LOG_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_LOG_H_

#include "base/logging.h"

#define PG_LOG(msg) LOG(INFO) << "*PageGraph* " << msg
#define PG_LOG_IF(condition, msg) << LOG_IF(INFO, condition) << msg

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_LOG_H_

