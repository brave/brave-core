/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/ui/autofill_image_fetcher.h"

#include "base/notreached.h"
#include "components/image_fetcher/core/request_metadata.h"
#include "ui/gfx/image/image.h"

namespace autofill {

AutofillImageFetcher::~AutofillImageFetcher() = default;

void AutofillImageFetcher::FetchCreditCardArtImagesForURLs(
    base::span<const GURL> image_urls,
    base::span<const AutofillImageFetcherBase::ImageSize> image_sizes_unused) {}

// Only implemented in Android clients. Pay with Pix is only available in
// Chrome on Android.
void AutofillImageFetcher::FetchPixAccountImagesForURLs(
    base::span<const GURL> image_urls) {
  NOTREACHED();
}

void AutofillImageFetcher::FetchValuableImagesForURLs(
    base::span<const GURL> image_urls) {}

const gfx::Image* AutofillImageFetcher::GetCachedImageForUrl(
    const GURL& image_url,
    ImageType image_type) const {
  return nullptr;
}

gfx::Image AutofillImageFetcher::ResolveCardArtImage(
    const GURL& card_art_url,
    const gfx::Image& card_art_image) {
  return card_art_image;
}

AutofillImageFetcher::AutofillImageFetcher() = default;

void AutofillImageFetcher::OnCardArtImageFetched(
    const GURL& card_art_url,
    const gfx::Image& card_art_image,
    const image_fetcher::RequestMetadata& metadata) {}

void AutofillImageFetcher::OnValuableImageFetched(
    const GURL& image_url,
    const gfx::Image& valuable_image,
    const image_fetcher::RequestMetadata& metadata) {}

}  // namespace autofill
