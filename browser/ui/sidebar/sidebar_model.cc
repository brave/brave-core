/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_model.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/time/time.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
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
  for (const auto& item : GetAllSidebarItems())
    AddItem(item, -1, false);

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
                           int index,
                           bool user_gesture) {
  if (index == -1) {
    data_.push_back(std::make_unique<SidebarModelData>(profile_));
  } else {
    data_.insert(data_.begin() + index,
                 std::make_unique<SidebarModelData>(profile_));
  }
  for (Observer& obs : observers_) {
    // Index starts at zero. If |index| is -1, add as a last item.
    obs.OnItemAdded(item, index == -1 ? data_.size() - 1 : index, user_gesture);
  }

  // If active_index_ is not -1, check this addition affetcs active index.
  if (active_index_ != -1 && active_index_ >= index)
    UpdateActiveIndexAndNotify(index);

  // Web type uses site favicon as button's image.
  if (sidebar::IsWebType(item))
    FetchFavicon(item);
}

void SidebarModel::OnItemAdded(const SidebarItem& item, int index) {
  AddItem(item, index, true);
}

void SidebarModel::OnItemMoved(const SidebarItem& item, int from, int to) {
  // Cache active model data to find its new index after moving.
  SidebarModelData* active_data = nullptr;
  if (active_index_ != -1) {
    active_data = data_[active_index_].get();
  }

  std::unique_ptr<SidebarModelData> data = std::move(data_[from]);
  data_.erase(data_.begin() + from);
  data_.insert(data_.begin() + to, std::move(data));

  for (Observer& obs : observers_)
    obs.OnItemMoved(item, from, to);

  if (!active_data)
    return;

  // Find new active items index.
  const int data_size = data_.size();
  for (int i = 0; i < data_size; ++i) {
    if (data_[i].get() == active_data) {
      UpdateActiveIndexAndNotify(i);
      return;
    }
  }

  NOTREACHED();
}

void SidebarModel::OnWillRemoveItem(const SidebarItem& item, int index) {
  if (index == active_index_)
    UpdateActiveIndexAndNotify(-1);
}

void SidebarModel::OnItemRemoved(const SidebarItem& item, int index) {
  RemoveItemAt(index);
}

void SidebarModel::OnURLVisited(history::HistoryService* history_service,
                                ui::PageTransition transition,
                                const history::URLRow& row,
                                const history::RedirectList& redirects,
                                base::Time visit_time) {
  const int item_count = GetAllSidebarItems().size();
  const auto items = GetAllSidebarItems();
  for (int i = 0; i < item_count; ++i) {
    // If same url is added to history service, try to fetch favicon to update
    // for item.
    if (items[i].url == row.url() && data_[i]->need_favicon_update()) {
      // Favicon seems cached after this callback.
      // TODO(simonhong): Find more deterministic method instead of using
      // delayed task.
      base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&SidebarModel::FetchFavicon,
                         weak_ptr_factory_.GetWeakPtr(), items[i]),
          base::TimeDelta::FromSeconds(2));
    }
  }
}

void SidebarModel::RemoveItemAt(int index) {
  data_.erase(data_.begin() + index);
  for (Observer& obs : observers_)
    obs.OnItemRemoved(index);

  if (active_index_ > index) {
    active_index_--;
    UpdateActiveIndexAndNotify(active_index_);
  }
}

void SidebarModel::SetActiveIndex(int index, bool load) {
  if (index == active_index_)
    return;

  // Don't load url if it's already loaded. If not, new loading is started
  // whenever item is activated.
  // TODO(simonhong): Maybe we should have reload option?
  if (load && index != -1 && !IsLoadedAt(index))
    LoadURLAt(GetAllSidebarItems()[index].url, index);

  UpdateActiveIndexAndNotify(index);
}

content::WebContents* SidebarModel::GetWebContentsAt(int index) {
  // Only webcontents is requested for items that opens in panel.
  // Opens in new tab doesn't need to get webcontents here.
  DCHECK(GetAllSidebarItems()[index].open_in_panel);

  return data_[index]->GetWebContents();
}

const std::vector<SidebarItem> SidebarModel::GetAllSidebarItems() const {
  return GetSidebarService(profile_)->items();
}

bool SidebarModel::IsLoadedAt(int index) const {
  DCHECK(GetAllSidebarItems()[index].open_in_panel);

  return data_[index]->IsLoaded();
}

bool SidebarModel::IsSidebarHasAllBuiltiInItems() const {
  return GetSidebarService(profile_)->GetNotAddedDefaultSidebarItems().empty();
}

int SidebarModel::GetIndexOf(const SidebarItem& item) const {
  const auto items = GetAllSidebarItems();
  const auto iter =
      std::find_if(items.begin(), items.end(),
                   [item](const auto& i) { return item.url == i.url; });
  if (iter == items.end())
    return -1;

  return std::distance(items.begin(), iter);
}

void SidebarModel::LoadURLAt(const GURL& url, int index) {
  DCHECK(GetAllSidebarItems()[index].open_in_panel);

  data_[index]->LoadURL(url);
}

void SidebarModel::UpdateActiveIndexAndNotify(int new_active_index) {
  if (new_active_index == active_index_)
    return;

  const int old_active_index = active_index_;
  active_index_ = new_active_index;

  for (Observer& obs : observers_)
    obs.OnActiveIndexChanged(old_active_index, active_index_);
}

void SidebarModel::FetchFavicon(const sidebar::SidebarItem& item) {
  // Use favicon as a web type icon's image.
  auto* favicon_service = FaviconServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
  favicon_service->GetRawFaviconForPageURL(
      item.url, {favicon_base::IconType::kFavicon}, 0 /*largest*/, false,
      base::BindRepeating(&SidebarModel::OnGetLocalFaviconImage,
                          weak_ptr_factory_.GetWeakPtr(), item),
      task_tracker_.get());
}

void SidebarModel::OnGetLocalFaviconImage(
    const sidebar::SidebarItem& item,
    const favicon_base::FaviconRawBitmapResult& bitmap_result) {
  const int index = GetIndexOf(item);
  if (index == -1)
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
    // Flaging to try to update favicon again.
    data_[index]->set_need_favicon_update(true);
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
