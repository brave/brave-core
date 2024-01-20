/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/file_system_access/file_system_access_handle_base.h"

#include "content/browser/file_system_access/file_system_access_directory_handle_impl.h"

#define DoRename DoRename_ChromiumImpl
#define DoMove DoMove_ChromiumImpl
#define GetChildURL(NEW_ENTRY_NAME, DEST_URL)                                 \
  GetChildURL(NEW_ENTRY_NAME, DEST_URL);                                      \
  if (dest_url.type() != storage::FileSystemType::kFileSystemTypeTemporary) { \
    std::move(callback).Run(file_system_access_error::FromStatus(             \
        blink::mojom::FileSystemAccessStatus::kNotSupportedError));           \
    return;                                                                   \
  }

#include "src/content/browser/file_system_access/file_system_access_handle_base.cc"
#undef GetChildURL
#undef DoMove
#undef DoRename

namespace content {

void FileSystemAccessHandleBase::DoMove(
    mojo::PendingRemote<blink::mojom::FileSystemAccessTransferToken>
        destination_directory,
    const std::string& new_entry_name,
    bool has_transient_user_activation,
    base::OnceCallback<void(blink::mojom::FileSystemAccessErrorPtr)> callback) {
  if (url().type() != storage::FileSystemType::kFileSystemTypeTemporary) {
    std::move(callback).Run(file_system_access_error::FromStatus(
        blink::mojom::FileSystemAccessStatus::kNotSupportedError));
    return;
  }

  DoMove_ChromiumImpl(std::move(destination_directory), new_entry_name,
                      has_transient_user_activation, std::move(callback));
}

void FileSystemAccessHandleBase::DoRename(
    const std::string& new_entry_name,
    bool has_transient_user_activation,
    base::OnceCallback<void(blink::mojom::FileSystemAccessErrorPtr)> callback) {
  if (url().type() != storage::FileSystemType::kFileSystemTypeTemporary) {
    std::move(callback).Run(file_system_access_error::FromStatus(
        blink::mojom::FileSystemAccessStatus::kNotSupportedError));
    return;
  }

  DoRename_ChromiumImpl(new_entry_name, has_transient_user_activation,
                        std::move(callback));
}

}  // namespace content
