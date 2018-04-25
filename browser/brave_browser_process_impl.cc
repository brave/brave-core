/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_process_impl.h"

#include "brave/browser/component_updater/brave_component_updater_configurator.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/https_everywhere_service.h"
#include "brave/components/brave_shields/browser/tracking_protection_service.h"
#include "chrome/browser/io_thread.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/browser_thread.h"

BraveBrowserProcessImpl* g_brave_browser_process = nullptr;

using content::BrowserThread;

BraveBrowserProcessImpl::~BraveBrowserProcessImpl() {
}

BraveBrowserProcessImpl::BraveBrowserProcessImpl(
    base::SequencedTaskRunner* local_state_task_runner)
    : BrowserProcessImpl(local_state_task_runner) {
  g_browser_process = this;
  g_brave_browser_process = this;
}

component_updater::ComponentUpdateService*
BraveBrowserProcessImpl::component_updater() {
  if (component_updater_)
    return component_updater_.get();

  if (!BrowserThread::CurrentlyOn(BrowserThread::UI))
    return nullptr;

  component_updater_ = component_updater::ComponentUpdateServiceFactory(
      component_updater::MakeBraveComponentUpdaterConfigurator(
          base::CommandLine::ForCurrentProcess(),
          io_thread()->system_url_request_context_getter(),
          g_browser_process->local_state()));

  return component_updater_.get();
}

brave_shields::AdBlockService*
BraveBrowserProcessImpl::ad_block_service() {
  if (ad_block_service_)
    return ad_block_service_.get();

  ad_block_service_ = brave_shields::AdBlockServiceFactory();
  return ad_block_service_.get();
}

brave_shields::TrackingProtectionService*
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
