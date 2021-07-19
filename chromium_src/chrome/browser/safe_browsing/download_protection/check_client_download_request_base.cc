/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/safe_browsing/core/common/proto/csd.pb.h"

#define BRAVE_ON_REQUEST_BUILT_FILTER \
  BraveFilterRequest(client_download_request_.get());

namespace safe_browsing {

void BraveFilterRequest(ClientDownloadRequest* request) {
  request->set_url("");  // URL must be present or we get a 400.
  request->clear_file_basename();
  request->clear_locale();
  request->clear_resources();  // Contains URLs and referrers
  request->clear_referrer_chain();

  // Filter binaries within archives.
  for (int i = 0; i < request->archived_binary_size(); i++) {
    ClientDownloadRequest_ArchivedBinary* archived_binary =
        request->mutable_archived_binary(i);
    archived_binary->clear_file_basename();
  }
}

}  // namespace safe_browsing

#include "../../../../../../chrome/browser/safe_browsing/download_protection/check_client_download_request_base.cc"  // NOLINT
