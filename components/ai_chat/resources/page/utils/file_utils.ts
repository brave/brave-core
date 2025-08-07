/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Mojom from '../../common/mojom'

// Utility function to convert File objects to UploadedFile format
export const convertFileToUploadedFile = (
  file: File
): Promise<Mojom.UploadedFile> => {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = (e) => {
      const arrayBuffer = e.target?.result as ArrayBuffer
      if (!arrayBuffer) {
        reject(new Error('Failed to read file'))
        return
      }

      const uint8Array = new Uint8Array(arrayBuffer)
      const uploadedFile: Mojom.UploadedFile = {
        filename: file.name,
        filesize: file.size,
        data: Array.from(uint8Array),
        type: Mojom.UploadedFileType.kImage
      }
      resolve(uploadedFile)
    }
    reader.onerror = () => reject(new Error('Failed to read file'))
    reader.readAsArrayBuffer(file)
  })
}