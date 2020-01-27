/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_source.h"

#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "url/gurl.h"

namespace {

typedef base::OnceCallback<void(BitmapFetcherService::RequestId request_id,
                                const GURL& url,
                                const SkBitmap& bitmap)>
    RewardsResourceFetcherCallback;

// Calls the specified callback when the requested image is downloaded.  This
// is a separate class instead of being implemented on BraveRewardsSource
// because BitmapFetcherService currently takes ownership of this object.
class RewardsResourceFetcherObserver : public BitmapFetcherService::Observer {
 public:
  explicit RewardsResourceFetcherObserver(
      const GURL& url,
      RewardsResourceFetcherCallback rewards_resource_fetcher_callback)
      : url_(url),
        rewards_resource_fetcher_callback_(
            std::move(rewards_resource_fetcher_callback)) {}

  void OnImageChanged(BitmapFetcherService::RequestId request_id,
                      const SkBitmap& image) override {
    DCHECK(!image.empty());
    // BitmapFetcherService does not invoke OnImageChanged more than once, in
    // spite of what the method name suggests.
    DCHECK(rewards_resource_fetcher_callback_);
    std::move(rewards_resource_fetcher_callback_).Run(request_id, url_, image);
  }

 private:
  GURL url_;
  RewardsResourceFetcherCallback rewards_resource_fetcher_callback_;

  DISALLOW_COPY_AND_ASSIGN(RewardsResourceFetcherObserver);
};

scoped_refptr<base::RefCountedMemory> BitmapToMemory(const SkBitmap* image) {
  base::RefCountedBytes* image_bytes = new base::RefCountedBytes;
  gfx::PNGCodec::EncodeBGRASkBitmap(*image, false, &image_bytes->data());
  return image_bytes;
}

}  // namespace

BraveRewardsSource::BraveRewardsSource(Profile* profile)
    : profile_(profile->GetOriginalProfile()) {}

BraveRewardsSource::~BraveRewardsSource() {
}

std::string BraveRewardsSource::GetSource() {
  return "rewards-image";
}

void BraveRewardsSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback got_data_callback) {
  if (!url.is_valid()) {
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  auto it = find(resource_fetchers_.begin(), resource_fetchers_.end(), url);
  if (it != resource_fetchers_.end()) {
    LOG(WARNING) << "Already fetching specified Brave Rewards resource, url: "
                 << url;
    return;
  }

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    net::NetworkTrafficAnnotationTag traffic_annotation =
        net::DefineNetworkTrafficAnnotation("brave_rewards_resource_fetcher", R"(
        semantics {
          sender:
            "Brave Rewards resource fetcher"
          description:
            "Fetches resources related to Brave Rewards."
          trigger:
            "User visits a media publisher's site."
          data: "Brave Rewards related resources."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");
    resource_fetchers_.emplace_back(url);
    request_ids_.push_back(image_service->RequestImage(
        url,
        // Image Service takes ownership of the observer.
        new RewardsResourceFetcherObserver(
            url, base::BindOnce(&BraveRewardsSource::OnBitmapFetched,
                                base::Unretained(this),
                                std::move(got_data_callback))),
        traffic_annotation));
  }
}

std::string BraveRewardsSource::GetMimeType(const std::string&) {
  // We need to explicitly return a mime type, otherwise if the user tries to
  // drag the image they get no extension.
  return "image/png";
}

bool BraveRewardsSource::AllowCaching() {
  return false;
}

bool BraveRewardsSource::ShouldReplaceExistingSource() {
  // Leave the existing DataSource in place, otherwise we'll drop any pending
  // requests on the floor.
  return false;
}

bool BraveRewardsSource::ShouldServiceRequest(
    const GURL& url,
    content::ResourceContext* resource_context,
    int render_process_id) {
  return URLDataSource::ShouldServiceRequest(url, resource_context,
                                             render_process_id);
}

void BraveRewardsSource::OnBitmapFetched(
    content::URLDataSource::GotDataCallback got_data_callback,
    BitmapFetcherService::RequestId request_id,
    const GURL& url,
    const SkBitmap& bitmap) {
  if (bitmap.isNull()) {
    LOG(ERROR) << "Failed to retrieve Brave Rewards resource, url: " << url;
    std::move(got_data_callback).Run(nullptr);
    return;
  }

  std::move(got_data_callback).Run(BitmapToMemory(&bitmap).get());

  auto it_url =
      find(resource_fetchers_.begin(), resource_fetchers_.end(), url);
  if (it_url != resource_fetchers_.end()) {
    resource_fetchers_.erase(it_url);
  }

  auto it_ids = find(request_ids_.begin(), request_ids_.end(), request_id);
  if (it_ids != request_ids_.end()) {
    request_ids_.erase(it_ids);
  }
}
