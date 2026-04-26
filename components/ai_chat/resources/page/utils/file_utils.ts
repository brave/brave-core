/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Mojom from '../../common/mojom'
import { ConversationContext } from '../state/conversation_context'

// Custom error types for better error handling
export class FileReadError extends Error {
  constructor(
    message: string,
    public readonly cause?: Error,
  ) {
    super(message)
    this.name = 'FileReadError'
  }
}

export class ImageProcessingError extends Error {
  constructor(
    message: string,
    public readonly cause?: Error,
  ) {
    super(message)
    this.name = 'ImageProcessingError'
  }
}

const fileTypeToUploadedFileType = (file: File): Mojom.UploadedFileType => {
  const mimeType = file.type.toLowerCase()
  if (mimeType === 'application/pdf') {
    return Mojom.UploadedFileType.kPdf
  } else if (mimeType.startsWith('image/')) {
    return Mojom.UploadedFileType.kImage
  } else {
    return Mojom.UploadedFileType.kText
  }
}

const toUploadedFile = async (file: File) => {
  const type = fileTypeToUploadedFileType(file)
  return {
    filename: file.name,
    filesize: file.size,
    data: Array.from(new Uint8Array(await file.arrayBuffer())),
    type,
    extractedText:
      type === Mojom.UploadedFileType.kText ? await file.text() : undefined,
  } as Mojom.UploadedFile
}

// Utility function to convert File objects to UploadedFile format
export const processFiles = async (
  files: File[],
  attachFiles: ConversationContext['attachFiles'],
) => {
  const uploadedFiles = await Promise.all(files.map(toUploadedFile))
  await attachFiles(uploadedFiles)
}
