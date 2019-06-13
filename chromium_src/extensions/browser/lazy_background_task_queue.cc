/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/browser/lazy_background_task_queue.h"

#undef LazyBackgroundTaskQueue
#define LazyBackgroundTaskQueue LazyBackgroundTaskQueue_Chromium

#include "../../../../extensions/browser/lazy_background_task_queue.cc"  // NOLINT

#undef LazyBackgroundTaskQueue

namespace extensions {

BraveLazyBackgroundTaskQueue::BraveLazyBackgroundTaskQueue(
    content::BrowserContext* browser_context)
  : LazyBackgroundTaskQueue_Chromium(browser_context) {
}

BraveLazyBackgroundTaskQueue::~BraveLazyBackgroundTaskQueue() {
}

void BraveLazyBackgroundTaskQueue::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  LazyBackgroundTaskQueue_Chromium::OnExtensionLoaded(
      browser_context, extension);

  if (!BackgroundInfo::HasLazyBackgroundPage(extension))
    return;

  ExtensionsBrowserClient* browser_client = ExtensionsBrowserClient::Get();
  if (browser_client->HasTorContext(browser_context)) {
    CreateLazyBackgroundHostOnExtensionLoaded(
        browser_client->GetTorContext(browser_context), extension);
  }
}

void BraveLazyBackgroundTaskQueue::NotifyTasksExtensionFailedToLoad(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  LazyBackgroundTaskQueue_Chromium::NotifyTasksExtensionFailedToLoad(
      browser_context,
      extension);

  ExtensionsBrowserClient* browser_client = ExtensionsBrowserClient::Get();
  if (browser_client->HasTorContext(browser_context)) {
    ProcessPendingTasks(nullptr,
                        browser_client->GetTorContext(browser_context),
                        extension);
  }
}

}  // namespace extensions
