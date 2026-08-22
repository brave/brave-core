// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/traffic_control/traffic_control_apply.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/traffic_control/traffic_control_tab_utils.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

namespace traffic_control {

TrafficControlApplier::TrafficControlApplier(
    content::WebContents* source,
    const GURL& url,
    mojom::TargetPtr target,
    std::optional<url::Origin> initiator_origin,
    ui::PageTransition page_transition)
    : source_(source),
      url_(url),
      target_(std::move(target)),
      initiator_origin_(std::move(initiator_origin)),
      page_transition_(page_transition) {}

TrafficControlApplier::~TrafficControlApplier() = default;

// static
bool TrafficControlApplier::AlreadyAtTarget(content::WebContents* web_contents,
                                            const mojom::Target& target) {
  if (!web_contents) {
    return false;
  }

  // Temporary destinations always open a new tab from non-temporary sources.
  // Once already in a temporary container, proceed to avoid an apply loop on
  // the newly opened tab's navigation.
  if (target.temporary_container) {
    return containers::IsTemporaryContainerId(
        containers::GetContainerIdForWebContents(web_contents));
  }

  // Unset destination: nothing to apply.
  if (!target.container_id.has_value()) {
    return true;
  }

  const std::string current =
      containers::GetContainerIdForWebContents(web_contents);
  if (target.container_id->empty()) {
    // Non-contained destination: already there if not in a container.
    return current.empty();
  }
  if (current == *target.container_id) {
    return true;
  }

  // Applier falls back to a non-contained tab when the container id is unknown.
  // Treat that as the effective destination so the replacement tab's navigation
  // is not cancelled and re-applied in a loop.
  if (current.empty()) {
    Profile* profile =
        Profile::FromBrowserContext(web_contents->GetBrowserContext());
    containers::ContainersService* containers_service =
        profile ? ContainersServiceFactory::GetForProfile(profile) : nullptr;
    if (containers_service &&
        !containers_service->GetRuntimeContainerById(*target.container_id)) {
      return true;
    }
  }
  return false;
}

// static
void TrafficControlApplier::Apply(base::WeakPtr<content::WebContents> source,
                                  const GURL& url,
                                  mojom::TargetPtr target,
                                  std::optional<url::Origin> initiator_origin,
                                  ui::PageTransition page_transition) {
  if (!source) {
    return;
  }
  TrafficControlApplier(source.get(), url, std::move(target),
                        std::move(initiator_origin), page_transition)
      .Run();
}

void TrafficControlApplier::Run() {
  CHECK(source_);
  CHECK(target_);
  if (!url_.is_valid()) {
    return;
  }
  if (!Initialize()) {
    return;
  }
  if (!OpenTargetTab()) {
    return;
  }
  MaybeCloseEmptySourceTab();
}

bool TrafficControlApplier::Initialize() {
  tabs::TabInterface* source_tab =
      tabs::TabInterface::MaybeGetFromContents(source_);
  if (!source_tab) {
    LOG(ERROR) << "Traffic Control: source tab not found";
    return false;
  }

  browser_window_ = source_tab->GetBrowserWindowInterface();
  if (!browser_window_) {
    LOG(ERROR) << "Traffic Control: browser window not found";
    return false;
  }

  profile_ = browser_window_->GetProfile();
  tab_strip_ = browser_window_->GetTabStripModel();
  close_source_after_ = IsDiscardableEmptyTab(source_);
  return true;
}

bool TrafficControlApplier::OpenTargetTab() {
  auto* containers_service = ContainersServiceFactory::GetForProfile(profile_);
  if (!containers_service) {
    LOG(ERROR) << "Traffic Control: ContainersService unavailable";
    return false;
  }

  if (target_->temporary_container) {
    OpenUrl(containers_service->CreateAndPersistTemporaryContainer().get());
    return true;
  }

  if (!target_->container_id.has_value()) {
    return false;
  }

  if (target_->container_id->empty()) {
    OpenUrl(/*container=*/nullptr);
    return true;
  }

  containers::mojom::ContainerPtr container =
      containers_service->GetRuntimeContainerById(*target_->container_id);
  if (!container) {
    LOG(WARNING) << "Traffic Control: unknown container id "
                 << *target_->container_id << ", opening without container";
    OpenUrl(/*container=*/nullptr);
    return true;
  }

  OpenUrl(container.get());
  return true;
}

void TrafficControlApplier::OpenUrl(
    const containers::mojom::Container* container) {
  const bool is_link =
      ui::PageTransitionCoreTypeIs(page_transition_, ui::PAGE_TRANSITION_LINK);
  NavigateParams params(
      browser_window_, url_,
      is_link ? ui::PAGE_TRANSITION_LINK : ui::PAGE_TRANSITION_TYPED);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.initiator_origin = initiator_origin_;
  if (container) {
    params.storage_partition_config = content::StoragePartitionConfig::Create(
        profile_, containers::kContainersStoragePartitionDomain, container->id,
        profile_->IsOffTheRecord());
  }
  Navigate(&params);
}

void TrafficControlApplier::MaybeCloseEmptySourceTab() {
  if (!close_source_after_ || !tab_strip_) {
    return;
  }

  // Re-resolve by pointer; indices can shift when the new tab is inserted.
  const int source_index_after = tab_strip_->GetIndexOfWebContents(source_);
  if (source_index_after != TabStripModel::kNoTab) {
    tab_strip_->CloseWebContentsAt(source_index_after,
                                   TabCloseTypes::CLOSE_NONE);
  }
}

}  // namespace traffic_control
