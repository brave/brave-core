/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process_impl.h"

#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"

BraveBrowserProcessImpl* g_brave_browser_process = nullptr;


BraveBrowserProcessImpl::~BraveBrowserProcessImpl() {
}

BraveBrowserProcessImpl::BraveBrowserProcessImpl(
    base::SequencedTaskRunner* local_state_task_runner)
    : BrowserProcessImpl(local_state_task_runner) {
  g_browser_process = this;
  g_brave_browser_process = this;
}

brave_shields::AdBlockService*
BraveBrowserProcessImpl::ad_block_service() {
  if (ad_block_service_)
    return ad_block_service_.get();

  ad_block_service_ = brave_shields::AdBlockServiceFactory();
  return ad_block_service_.get();
}

brave_shields::BaseBraveShieldsService*
BraveBrowserProcessImpl::tracking_protection_service() {
  if (tracking_protection_service_)
    return tracking_protection_service_.get();

  tracking_protection_service_ =
    brave_shields::TrackingProtectionServiceFactory();
  return tracking_protection_service_.get();
}

brave_shields::HTTPSEverywhereService*
BraveBrowserProcessImpl::https_everywhere_service() {
  if (https_everywhere_service_)
    return https_everywhere_service_.get();

  https_everywhere_service_ =
    brave_shields::HTTPSEverywhereServiceFactory();
  return https_everywhere_service_.get();
}
