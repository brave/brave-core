/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_LAZY_BACKGROUND_TASK_QUEUE_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_LAZY_BACKGROUND_TASK_QUEUE_H_

#define LazyBackgroundTaskQueue LazyBackgroundTaskQueue_Chromium
#include "../../../../extensions/browser/lazy_background_task_queue.h"
#undef LazyBackgroundTaskQueue

namespace extensions {

class BraveLazyBackgroundTaskQueue : public LazyBackgroundTaskQueue_Chromium {
  public:
    explicit BraveLazyBackgroundTaskQueue(
        content::BrowserContext* browser_context);
    ~BraveLazyBackgroundTaskQueue() override;

  private:
    void OnExtensionLoaded(content::BrowserContext* browser_context,
                           const Extension* extension) override;

    void NotifyTasksExtensionFailedToLoad(
        content::BrowserContext* browser_context,
        const Extension* extension);

    DISALLOW_COPY_AND_ASSIGN(BraveLazyBackgroundTaskQueue);
};

}  // namespace extensions

// Use our own subclass as the real LazyBackgroundTaskQueue
#define LazyBackgroundTaskQueue BraveLazyBackgroundTaskQueue

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_LAZY_BACKGROUND_TASK_QUEUE_H_
