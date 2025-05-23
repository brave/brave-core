/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_UI_AUTOFILL_IMAGE_FETCHER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_UI_AUTOFILL_IMAGE_FETCHER_H_

#include "components/autofill/core/browser/ui/autofill_image_fetcher_base.h"

class GURL;

namespace gfx {
class Image;
}  // namespace gfx

namespace image_fetcher {
class ImageFetcher;
struct RequestMetadata;
}  // namespace image_fetcher

namespace autofill {

// Stub out to prevent getting images from a Google server. This is subclassed
// by AutofillImageFetcherImpl on Desktop and iOS.
class AutofillImageFetcher : public AutofillImageFetcherBase {
 public:
  ~AutofillImageFetcher() override;

  // AutofillImageFetcherBase:
  void FetchCreditCardArtImagesForURLs(
      base::span<const GURL> image_urls,
      base::span<const AutofillImageFetcherBase::ImageSize> image_sizes_unused)
      override;
  void FetchPixAccountImagesForURLs(base::span<const GURL> image_urls) override;
  void FetchValuableImagesForURLs(base::span<const GURL> image_urls) override;
  const gfx::Image* GetCachedImageForUrl(const GURL& image_url,
                                         ImageType image_type) const override;

  virtual gfx::Image ResolveCardArtImage(const GURL& card_art_url,
                                         const gfx::Image& card_art_image);

  // Implemented in subclasses
  virtual GURL ResolveImageURL(const GURL& card_art_url,
                               ImageType image_type) const = 0;
  virtual image_fetcher::ImageFetcher* GetImageFetcher() = 0;
  virtual base::WeakPtr<AutofillImageFetcher> GetWeakPtr() = 0;

 protected:
  AutofillImageFetcher();

  // These are needed for by upstream unit tests.
  void OnCardArtImageFetched(const GURL& card_art_url,
                             const gfx::Image& card_art_image,
                             const image_fetcher::RequestMetadata& metadata);

  void OnValuableImageFetched(const GURL& image_url,
                              const gfx::Image& valuable_image,
                              const image_fetcher::RequestMetadata& metadata);
};

}  // namespace autofill

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_UI_AUTOFILL_IMAGE_FETCHER_H_
