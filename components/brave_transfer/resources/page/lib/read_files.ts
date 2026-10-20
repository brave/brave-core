/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { FileData } from '../api/transfer_api'

// Files dropped on or picked from the page are copied into the mojo message,
// so their total size is capped. Larger files are sent with the native picker
// (PickAndSend), which lets the host read them from disk.
export const maxPageFileBytes = 64 * 1024 * 1024

export function filesFitInMessage(files: File[]) {
  const total = files.reduce((sum, file) => sum + file.size, 0)
  return total <= maxPageFileBytes
}

export async function readFiles(files: File[]): Promise<FileData[]> {
  return Promise.all(
    files.map(async (file) => ({
      name: file.name,
      mimeType: file.type,
      data: Array.from(new Uint8Array(await file.arrayBuffer())),
    })),
  )
}
