// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from './mojom'

/**
 * Checks if a file is a full page screenshot
 * @param file - The uploaded file to check
 * @returns True if the file is a full page screenshot
 */
export const isFullPageScreenshot = (file: Mojom.UploadedFile): boolean => {
  return (
    file.type === Mojom.UploadedFileType.kScreenshot
    && file.filename.startsWith(Mojom.FULL_PAGE_SCREENSHOT_PREFIX)
  )
}

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
  files: Mojom.UploadedFile[] | null,
): Mojom.UploadedFile[] | undefined {
  return files?.filter(
    (file) =>
      file.type === Mojom.UploadedFileType.kImage
      || file.type === Mojom.UploadedFileType.kScreenshot,
  )
}

/**
 * Filters uploaded files to only include documents
 *
 * @param files - The array of uploaded files to filter
 * @returns Filtered array containing only document files
 */
export function getDocumentFiles(
  files: Mojom.UploadedFile[] | null,
): Mojom.UploadedFile[] | undefined {
  return files?.filter((file) => file.type === Mojom.UploadedFileType.kPdf)
}

/**
 * Determines if the attachment button should be disabled based on current file limits
 *
 * @param conversationHistory - The conversation history to check uploaded files
 * @returns true if attachment button should be disabled, false otherwise
 */
export function shouldDisableAttachmentsButton(
  conversationHistory: Mojom.ConversationTurn[],
): boolean {
  const totalUploadedImages = conversationHistory.reduce(
    (total, turn) => total + (getImageFiles(turn.uploadedFiles)?.length || 0),
    0,
  )

  const totalUploadedDocuments = conversationHistory.reduce(
    (total, turn) =>
      total + (getDocumentFiles(turn.uploadedFiles)?.length || 0),
    0,
  )

  return (
    totalUploadedImages >= Mojom.MAX_IMAGES
    || totalUploadedDocuments >= Mojom.MAX_DOCUMENTS
  )
}

/**
 * Process uploaded files with limits to different types
 *
 * @param files - The uploaded file to be processed
 * @param conversationHistory - The current conversation history
 * @param currentPendingFiles - The current files in the staging area
 * @returns The files the user can upload after checking limits
 */
export const processUploadedFilesWithLimits = (
  files: Mojom.UploadedFile[],
  conversationHistory: Mojom.ConversationTurn[],
  currentPendingFiles: Mojom.UploadedFile[],
): Mojom.UploadedFile[] => {
  // Calculate total uploaded files from conversation history
  const totalUploadedImages = conversationHistory.reduce(
    (total, turn) => total + (getImageFiles(turn.uploadedFiles)?.length || 0),
    0,
  )
  const totalUploadedDocuments = conversationHistory.reduce(
    (total, turn) =>
      total + (getDocumentFiles(turn.uploadedFiles)?.length || 0),
    0,
  )

  // Calculate current pending files by type
  const currentPendingImages = getImageFiles(currentPendingFiles)?.length || 0
  const currentPendingDocuments =
    getDocumentFiles(currentPendingFiles)?.length || 0

  // Track current counts for each type
  let currentImages = 0
  let currentDocuments = 0
  // Process files in original order while respecting limits
  const newFiles: Mojom.UploadedFile[] = []
  for (const file of files) {
    const isImage =
      file.type === Mojom.UploadedFileType.kImage
      || file.type === Mojom.UploadedFileType.kScreenshot
    const isDocument = file.type === Mojom.UploadedFileType.kPdf
    if (isImage) {
      const maxNewImages =
        Mojom.MAX_IMAGES - totalUploadedImages - currentPendingImages
      if (currentImages < maxNewImages) {
        newFiles.push(file)
        currentImages++
      }
    } else if (isDocument) {
      const maxNewDocuments =
        Mojom.MAX_DOCUMENTS - totalUploadedDocuments - currentPendingDocuments
      const fileSize = Number(file.filesize)
      if (
        currentDocuments < maxNewDocuments
        && fileSize <= Mojom.MAX_DOCUMENT_SIZE_BYTES
      ) {
        newFiles.push(file)
        currentDocuments++
      }
    }
  }

  return newFiles
}
