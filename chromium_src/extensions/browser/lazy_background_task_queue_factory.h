/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_LAZY_BACKGROUND_TASK_QUEUE_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_LAZY_BACKGROUND_TASK_QUEUE_FACTORY_H_

// Include lazy_background_task_queue.h will substitute LazyBackgroundTaskQueue
// with BraveLazyBackgroundTaskQueue because of our chromium_src override of
// lazy_background_task_queue.h.
#include "extensions/browser/lazy_background_task_queue.h"
#include "../../../../extensions/browser/lazy_background_task_queue_factory.h"

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_LAZY_BACKGROUND_TASK_QUEUE_FACTORY_H_
