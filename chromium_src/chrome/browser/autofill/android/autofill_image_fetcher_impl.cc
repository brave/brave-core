/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/autofill/android/autofill_image_fetcher_impl.h"

#define AutofillImageFetcherImpl AutofillImageFetcherImpl_ChromiumImpl
#include "src/chrome/browser/autofill/android/autofill_image_fetcher_impl.cc"
#undef AutofillImageFetcherImpl

namespace autofill {

AutofillImageFetcherImpl::~AutofillImageFetcherImpl() = default;

void AutofillImageFetcherImpl::FetchCreditCardArtImagesForURLs(
    base::span<const GURL> image_urls,
    base::span<const AutofillImageFetcherBase::ImageSize> image_sizes) {}

void AutofillImageFetcherImpl::FetchPixAccountImagesForURLs(
    base::span<const GURL> image_urls) {}

void AutofillImageFetcherImpl::FetchValuableImagesForURLs(
    base::span<const GURL> image_urls) {}

const gfx::Image* AutofillImageFetcherImpl::GetCachedImageForUrl(
    const GURL& image_url,
    ImageType image_type) const {
  return nullptr;
}

}  // namespace autofill
