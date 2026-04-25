/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_URL_STORAGE_CHECKER_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_URL_STORAGE_CHECKER_H_

#include <vector>

#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/cookies/canonical_cookie.h"
#include "third_party/blink/public/mojom/dom_storage/storage_area.mojom.h"
#include "url/gurl.h"

namespace content {
class StoragePartition;
}  // namespace content

namespace ephemeral_storage {

// Performs cookies and localStorage data existence check for a URL.
class UrlStorageChecker : public base::RefCounted<UrlStorageChecker> {
 public:
  using Callback = base::OnceCallback<void(bool is_storage_empty)>;

  UrlStorageChecker(content::StoragePartition& storage_partition,
                    const GURL& url,
                    Callback callback);

  void StartCheck();

 private:
  friend class base::RefCounted<UrlStorageChecker>;
  ~UrlStorageChecker();

  void OnGetCookieList(
      const std::vector<net::CookieWithAccessResult>& included_cookies,
      const std::vector<net::CookieWithAccessResult>& excluded_cookies);

  void OnGetLocalStorageData(
      std::vector<blink::mojom::KeyValuePtr> local_storage_data);

  const raw_ref<content::StoragePartition> storage_partition_;
  GURL url_;
  Callback callback_;

  mojo::Remote<blink::mojom::StorageArea> local_storage_area_;
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_URL_STORAGE_CHECKER_H_
