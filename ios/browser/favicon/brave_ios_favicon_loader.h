/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_H_
#define BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/keyed_service/core/keyed_service.h"

namespace favicon {
class FaviconService;
}  // namespace favicon

class GURL;
@class FaviconAttributes;

namespace brave_favicon {

class BraveFaviconLoader : public KeyedService {
 public:
  // Type for completion block for FaviconForURL().
  typedef void (^FaviconAttributesCompletionBlock)(FaviconAttributes*);

  explicit BraveFaviconLoader(favicon::FaviconService* favicon_service);

  BraveFaviconLoader(const BraveFaviconLoader&) = delete;
  BraveFaviconLoader& operator=(const BraveFaviconLoader&) = delete;

  ~BraveFaviconLoader() override;

  void FaviconForPageUrlOrHost(
      const GURL& page_url,
      float size_in_points,
      float min_size_in_points,
      FaviconAttributesCompletionBlock faviconBlockHandler);

  // Cancel all incomplete requests.
  void CancellAllRequests();

  // Return a weak pointer to the current object.
  base::WeakPtr<BraveFaviconLoader> AsWeakPtr();

 private:
  base::raw_ptr<favicon::FaviconService> favicon_service_;

  // Tracks tasks sent to HistoryService.
  base::CancelableTaskTracker cancelable_task_tracker_;

  base::WeakPtrFactory<BraveFaviconLoader> weak_ptr_factory_{this};
};

}  // namespace brave_favicon

#endif  // BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_H_
