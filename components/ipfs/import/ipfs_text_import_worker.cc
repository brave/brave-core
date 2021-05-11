/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/import/ipfs_text_import_worker.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_network_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "net/base/mime_util.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "url/gurl.h"

namespace ipfs {

IpfsTextImportWorker::IpfsTextImportWorker(content::BrowserContext* context,
                                           const GURL& endpoint,
                                           ImportCompletedCallback callback,
                                           const std::string& text,
                                           const std::string& host)
    : IpfsImportWorkerBase(context, endpoint, std::move(callback)) {
  ImportText(text, host);
}

IpfsTextImportWorker::~IpfsTextImportWorker() = default;




}  // namespace ipfs
