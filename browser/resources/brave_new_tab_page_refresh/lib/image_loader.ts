/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const placeholderImageSrc =
  'data:image/svg+xml,%3Csvg xmlns="http://www.w3.org/2000/svg"%3E%3C/svg%3E'

// Loads an image in the background and resolves when it has loaded.
// Returns a blob URL if successful, rejects if failed.
export function loadImageAsBlob(url: string): Promise<string> {
  return new Promise((resolve, reject) => {
    if (!url) {
      reject(new Error('No URL provided'))
      return
    }

    fetch(url)
      .then(response => {
        if (!response.ok) {
          reject(new Error(`image fetch failed: ${response.statusText}`))
          return
        }
        return response.blob()
      })
      .then(blob => {
        if (!blob) {
          reject(new Error('Failed to get blob from response'))
          return
        }
        // Create a blob URL that can be used directly
        const blobUrl = URL.createObjectURL(blob)
        resolve(blobUrl)
      })
      .catch(error => {
        reject(error)
      })
  })
}
