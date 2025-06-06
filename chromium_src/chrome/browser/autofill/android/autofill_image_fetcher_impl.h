/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_AUTOFILL_ANDROID_AUTOFILL_IMAGE_FETCHER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_AUTOFILL_ANDROID_AUTOFILL_IMAGE_FETCHER_IMPL_H_

#include "components/autofill/core/browser/ui/autofill_image_fetcher_base.h"

// Prevent getting images from a Google server.
#define AutofillImageFetcherImpl AutofillImageFetcherImpl_ChromiumImpl
#include "src/chrome/browser/autofill/android/autofill_image_fetcher_impl.h"  // IWYU pragma: export
#undef AutofillImageFetcherImpl

namespace autofill {

class AutofillImageFetcherImpl : public AutofillImageFetcherImpl_ChromiumImpl {
 public:
  using AutofillImageFetcherImpl_ChromiumImpl::
      AutofillImageFetcherImpl_ChromiumImpl;
  ~AutofillImageFetcherImpl() override;

  // AutofillImageFetcherBase:
  void FetchCreditCardArtImagesForURLs(
      base::span<const GURL> image_urls,
      base::span<const AutofillImageFetcherBase::ImageSize> image_sizes)
      override;
  void FetchPixAccountImagesForURLs(base::span<const GURL> image_urls) override;
  void FetchValuableImagesForURLs(base::span<const GURL> image_urls) override;
  const gfx::Image* GetCachedImageForUrl(const GURL& image_url,
                                         ImageType image_type) const override;
};

}  // namespace autofill

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_AUTOFILL_ANDROID_AUTOFILL_IMAGE_FETCHER_IMPL_H_
