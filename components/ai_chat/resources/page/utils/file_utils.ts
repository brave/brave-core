/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as Mojom from '../../common/mojom'
import getAPI from '../api'

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

export class UnsupportedFileTypeError extends Error {
  constructor(message: string) {
    super(message)
    this.name = 'UnsupportedFileTypeError'
  }
}

// Utility function to convert File objects to UploadedFile format
export const convertFileToUploadedFile = async (
  file: File,
): Promise<Mojom.UploadedFile> => {
  const reader = new FileReader()
  const arrayBuffer = await new Promise<ArrayBuffer>((resolve, reject) => {
    reader.onload = (e) => {
      const result = e.target?.result as ArrayBuffer
      if (!result) {
        reject(new FileReadError('Failed to read file: No data received'))
        return
      }
      resolve(result)
    }
    reader.onerror = () =>
      reject(new FileReadError(`Failed to read file: ${file.name}`))
    reader.readAsArrayBuffer(file)
  })

  const uint8Array = new Uint8Array(arrayBuffer)

  // Check file type and handle accordingly
  const mimeType = file.type.toLowerCase()
  if (mimeType === 'application/pdf') {
    // Handle PDF files directly
    const uploadedFile: Mojom.UploadedFile = {
      filename: file.name,
      filesize: file.size,
      data: Array.from(uint8Array),
      type: Mojom.UploadedFileType.kPdf,
    }
    return uploadedFile
  } else if (mimeType.startsWith('image/')) {
    // Use backend processing for images via mojo call
    try {
      const api = getAPI()
      const response = await api.uiHandler.processImageFile(
        Array.from(uint8Array),
        file.name,
      )

      if (!response.processedFile) {
        throw new ImageProcessingError(
          'Failed to process image file: Backend returned no result',
        )
      }

      return response.processedFile
    } catch (error) {
      if (error instanceof ImageProcessingError) {
        throw error
      }

      // Re-throw any other errors as-is
      throw error
    }
  } else {
    throw new UnsupportedFileTypeError(
      `Unsupported file type: ${file.type}. Only images and PDF files `
        + `are supported.`,
    )
  }
}
