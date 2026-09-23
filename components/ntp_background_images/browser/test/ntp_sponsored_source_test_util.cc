/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_source_test_util.h"

#include <string>

#include "base/base_paths.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/path_service.h"
#include "base/strings/string_view_util.h"
#include "base/test/test_future.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace ntp_background_images::test {

base::FilePath GetSponsoredImagesComponentPath() {
  return base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
      .AppendASCII("brave")
      .AppendASCII("test")
      .AppendASCII("data")
      .AppendASCII("components")
      .AppendASCII("ntp_sponsored_images");
}

std::string StartDataRequest(content::URLDataSource* url_data_source,
                             const GURL& url) {
  CHECK(url_data_source);

  content::WebContents::Getter wc_getter;
  base::test::TestFuture<scoped_refptr<base::RefCountedMemory>> test_future;
  url_data_source->StartDataRequest(url, wc_getter, test_future.GetCallback());
  const scoped_refptr<base::RefCountedMemory> bytes = test_future.Get();
  return bytes ? std::string(base::as_string_view(*bytes)) : std::string();
}

}  // namespace ntp_background_images::test
