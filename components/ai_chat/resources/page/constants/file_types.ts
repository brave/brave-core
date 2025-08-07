/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Supported image file types for AI Chat uploads (matches C++ backend)
const SUPPORTED_IMAGE_TYPES: string[] = [
  'image/png',
  'image/jpeg',
  'image/webp'
]

// Check if file is a supported image format
export const isImageFile = (file: File): boolean => {
  return SUPPORTED_IMAGE_TYPES.includes(file.type.toLowerCase())
}
