/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/autofill/popup/popup_view_views.h"

#include <memory>
#include <utility>

#include "base/functional/callback.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/autofill/popup/password_favicon_loader.h"

namespace autofill {

namespace {

// Password rows in off-the-record profiles, including Tor windows, keep their
// placeholder icon. `PasswordFaviconLoaderImpl` fetches favicons through the
// browser-wide system network context or through the original profile, so the
// request would leave the off-the-record profile's network partition, and in a
// Tor window it would bypass the Tor proxy.
class OffTheRecordPasswordFaviconLoader : public PasswordFaviconLoaderImpl {
 public:
  using PasswordFaviconLoaderImpl::PasswordFaviconLoaderImpl;

  void Load(const Suggestion::FaviconDetails& favicon_details,
            base::CancelableTaskTracker* task_tracker,
            OnLoadSuccess on_success,
            OnLoadFail on_fail) override {
    std::move(on_fail).Run();
  }
};

std::unique_ptr<PasswordFaviconLoaderImpl> CreatePasswordFaviconLoader(
    Profile* profile,
    favicon::LargeIconService* favicon_service,
    image_fetcher::ImageFetcher* image_fetcher) {
  if (profile->IsOffTheRecord()) {
    return std::make_unique<OffTheRecordPasswordFaviconLoader>(favicon_service,
                                                               image_fetcher);
  }
  return std::make_unique<PasswordFaviconLoaderImpl>(favicon_service,
                                                     image_fetcher);
}

}  // namespace

}  // namespace autofill

#include <chrome/browser/ui/views/autofill/popup/popup_view_views.cc>
