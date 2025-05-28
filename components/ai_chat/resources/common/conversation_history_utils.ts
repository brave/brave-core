// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

/**
 * Updates the conversation history by either merging a new entry with an
 * existing one or appending it if it doesn't exist.
 *
 * @param currentHistory - The current conversation history
 * @param newEntry - The new entry to be merged or appended
 * @returns Updated conversation history
 */
export function updateConversationHistory(
  currentHistory: Mojom.ConversationTurn[],
  newEntry: Mojom.ConversationTurn,
): Mojom.ConversationTurn[] {
  // Check if an entry with the same UUID already exists
  const existingEntryIndex = currentHistory.findIndex(
    (existingEntry) => existingEntry.uuid === newEntry.uuid,
  )

  if (existingEntryIndex !== -1) {
    // If entry exists, merge it with the existing one
    const updatedHistory = [...currentHistory]
    updatedHistory[existingEntryIndex] = {
      ...updatedHistory[existingEntryIndex],
      ...newEntry,
    }
    return updatedHistory
  } else {
    // If entry doesn't exist, append it
    return [...currentHistory, newEntry]
  }
}

/**
 * Filters uploaded files to only include images and screenshots
 *
 * @param files - The array of uploaded files to filter
 * @returns Filtered array containing only image and screenshot files
 */
export function getImageFiles(
  files?: Mojom.UploadedFile[],
): Mojom.UploadedFile[] | undefined {
  return files?.filter(
    (file) =>
      file.type === Mojom.UploadedFileType.kImage
      || file.type === Mojom.UploadedFileType.kScreenshot,
  )
}
