/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_model.h"

#include <optional>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/image_fetcher/image_fetcher_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_key.h"
#include "components/favicon/core/favicon_service.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/image_fetcher/core/image_fetcher_service.h"
#include "components/search_engines/template_url.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace sidebar {

namespace {

SidebarService* GetSidebarService(Profile* profile) {
  auto* service = SidebarServiceFactory::GetForProfile(profile);
  DCHECK(service);

  return service;
}

constexpr char kImageFetcherUmaClientName[] = "SidebarFavicon";

constexpr net::NetworkTrafficAnnotationTag kSidebarFaviconTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("sidebar_model", R"(
      semantics {
        sender: "Sidebar"
        description:
          "Fetches favicon for web type sidebar item"
        trigger:
          "When web type sidebar item is added to sidebar"
        data: "URL of the favicon image to be fetched."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting: "Disabled when the user disabled sidebar."
      })");

}  // namespace

SidebarModel::SidebarModel(Profile* profile)
    : profile_(profile), task_tracker_(new base::CancelableTaskTracker{}) {}

SidebarModel::~SidebarModel() = default;

void SidebarModel::Init(history::HistoryService* history_service) {
  // Start with saved item list.
  int index = 0u;
  for (const auto& item : GetAllSidebarItems())
    AddItem(item, index++, false);

  sidebar_observed_.Observe(GetSidebarService(profile_));
  // Can be null in test.
  if (history_service)
    history_observed_.Observe(history_service);
}

void SidebarModel::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SidebarModel::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SidebarModel::AddItem(const SidebarItem& item,
                           size_t index,
                           bool user_gesture) {
  // Sidebar service should always call with a valid index equal to the
  // index of the SidebarItem.
  for (Observer& obs : observers_) {
    obs.OnItemAdded(item, index, user_gesture);
  }

  // Check this addition affects active index.
  if (active_index_ >= index) {
    UpdateActiveIndexAndNotify(*active_index_ + 1);
  }

  // Web type uses site favicon as button's image.
  if (item.IsWebType()) {
    FetchFavicon(item);
  }
}

void SidebarModel::OnItemAdded(const SidebarItem& item, size_t index) {
  AddItem(item, index, true);
}

void SidebarModel::OnItemMoved(const SidebarItem& item,
                               size_t from,
                               size_t to) {
  for (Observer& obs : observers_)
    obs.OnItemMoved(item, from, to);

  if (!active_index_)
    return;

  // Find new active items index.
  const bool active_index_is_unaffected =
      ((active_index_ > from && active_index_ > to) ||
       (active_index_ < from && active_index_ < to));
  if (active_index_is_unaffected) {
    return;
  }
  std::optional<size_t> new_active_index = std::nullopt;
  if (active_index_ == from) {
    new_active_index = to;
  } else {
    new_active_index = (to < from) ? *active_index_ + 1 : *active_index_ - 1;
  }
  DCHECK(new_active_index);
  UpdateActiveIndexAndNotify(new_active_index);
}

void SidebarModel::OnItemUpdated(const SidebarItem& item,
                                 const SidebarItemUpdate& update) {
  for (Observer& obs : observers_)
    obs.OnItemUpdated(item, update);

  // New url needs its favicon.
  if (update.url_updated)
    FetchFavicon(item);
}

void SidebarModel::OnWillRemoveItem(const SidebarItem& item, size_t index) {
  if (index == active_index_)
    UpdateActiveIndexAndNotify(std::nullopt);

  for (Observer& obs : observers_) {
    obs.OnWillRemoveItem(item);
  }
}

void SidebarModel::OnItemRemoved(const SidebarItem& item, size_t index) {
  RemoveItemAt(index);
}

void SidebarModel::OnURLVisited(history::HistoryService* history_service,
                                const history::URLRow& url_row,
                                const history::VisitRow& new_visit) {
  for (const auto& item : GetAllSidebarItems()) {
    // Don't try to update builtin items image. It uses bundled one.
    if (item.IsBuiltInType()) {
      continue;
    }

    // If same url is added to history service, try to fetch favicon to update
    // for item.
    if (item.url.host() == url_row.url().host()) {
      // Favicon seems cached after this callback.
      // TODO(simonhong): Find more deterministic method instead of using
      // delayed task.
      base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&SidebarModel::FetchFavicon,
                         weak_ptr_factory_.GetWeakPtr(), item),
          base::Seconds(2));
    }
  }
}

void SidebarModel::RemoveItemAt(size_t index) {
  for (Observer& obs : observers_)
    obs.OnItemRemoved(index);

  if (active_index_ > index) {
    --(*active_index_);
    UpdateActiveIndexAndNotify(active_index_);
  }
}

void SidebarModel::SetActiveIndex(std::optional<size_t> index) {
  if (index == active_index_)
    return;

  UpdateActiveIndexAndNotify(index);
}

const std::vector<SidebarItem>& SidebarModel::GetAllSidebarItems() const {
  return GetSidebarService(profile_)->items();
}

bool SidebarModel::IsSidebarHasAllBuiltInItems() const {
  return GetSidebarService(profile_)->GetHiddenDefaultSidebarItems().empty();
}

std::optional<size_t> SidebarModel::GetIndexOf(const SidebarItem& item) const {
  const auto& items = GetAllSidebarItems();
  const auto iter = base::ranges::find_if(items, [&item](const auto& i) {
    return (item.built_in_item_type == i.built_in_item_type &&
            item.url == i.url);
  });
  if (iter == items.end())
    return std::nullopt;

  return std::distance(items.begin(), iter);
}

std::optional<size_t> SidebarModel::GetIndexOf(
    SidebarItem::BuiltInItemType type) const {
  const auto& items = GetAllSidebarItems();
  const auto iter = base::ranges::find_if(items, [&type](const auto& i) {
    return i.IsBuiltInType() && (type == i.built_in_item_type);
  });
  if (iter == items.end()) {
    return std::nullopt;
  }

  return std::distance(items.begin(), iter);
}

void SidebarModel::UpdateActiveIndexAndNotify(
    std::optional<size_t> new_active_index) {
  if (new_active_index == active_index_)
    return;

  auto old_active_index = active_index_;
  active_index_ = new_active_index;

  for (Observer& obs : observers_)
    obs.OnActiveIndexChanged(old_active_index, active_index_);
}

void SidebarModel::FetchFavicon(const sidebar::SidebarItem& item) {
  // Use favicon as a web type icon's image.
  auto* favicon_service = FaviconServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
  // nullptr in unit test.
  if (!favicon_service)
    return;
  favicon_service->GetRawFaviconForPageURL(
      item.url, {favicon_base::IconType::kFavicon}, 0 /*largest*/, false,
      base::BindRepeating(&SidebarModel::OnGetLocalFaviconImage,
                          weak_ptr_factory_.GetWeakPtr(), item),
      task_tracker_.get());
}

void SidebarModel::OnGetLocalFaviconImage(
    const sidebar::SidebarItem& item,
    const favicon_base::FaviconRawBitmapResult& bitmap_result) {
  auto index = GetIndexOf(item);
  if (!index)
    return;

  // Fetch favicon from local favicon service.
  // If history is cleared, favicon service can't give. Then, try to get from
  // network.
  if (bitmap_result.is_valid()) {
    for (Observer& obs : observers_) {
      obs.OnFaviconUpdatedForItem(
          item, gfx::Image::CreateFrom1xPNGBytes(bitmap_result.bitmap_data)
                    .AsImageSkia());
    }
  } else {
    FetchFaviconFromNetwork(item);
  }
}

void SidebarModel::FetchFaviconFromNetwork(const sidebar::SidebarItem& item) {
  auto* service =
      ImageFetcherServiceFactory::GetForKey(profile_->GetProfileKey());
  DCHECK(service);
  auto* fetcher = service->GetImageFetcher(
      image_fetcher::ImageFetcherConfig::kDiskCacheOnly);
  image_fetcher::ImageFetcherParams params(kSidebarFaviconTrafficAnnotation,
                                           kImageFetcherUmaClientName);
  fetcher->FetchImage(
      TemplateURL::GenerateFaviconURL(item.url),
      base::BindOnce(&SidebarModel::OnGetFaviconImageFromNetwork,
                     weak_ptr_factory_.GetWeakPtr(), item),
      params);
}

void SidebarModel::OnGetFaviconImageFromNetwork(
    const sidebar::SidebarItem& item,
    const gfx::Image& image,
    const image_fetcher::RequestMetadata& request_metadata) {
  if (!image.IsEmpty()) {
    for (Observer& obs : observers_)
      obs.OnFaviconUpdatedForItem(item, image.AsImageSkia());
  }
}

}  // namespace sidebar
