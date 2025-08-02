// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { processUploadedFilesWithLimits } from './conversation_context'
import * as Mojom from '../../common/mojom'
import { MAX_IMAGES, MAX_DOCUMENTS, MAX_DOCUMENT_SIZE_BYTES } from '../../common/constants'

describe('processUploadedFilesWithLimits', () => {
  const createMockFile = (
    type: Mojom.UploadedFileType,
    filesize: number,
    filename: string = 'test-file'
  ): Mojom.UploadedFile => ({
    type,
    filesize: filesize,
    filename,
    data: [1, 2, 3, 4]
  })

  it('should process files in original order while respecting limits', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'doc2.pdf')
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(files, conversationHistory, currentPendingFiles)

    // Should accept all files since we're under limits
    expect(result).toHaveLength(4)
    expect(result[0].filename).toBe('image1.jpg')
    expect(result[1].filename).toBe('doc1.pdf')
    expect(result[2].filename).toBe('image2.jpg')
    expect(result[3].filename).toBe('doc2.pdf')
  })

  it('should respect image count limit', () => {
    // Create more images than the limit
    const files = Array.from({ length: MAX_IMAGES + 5 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, 1024, `image${i}.jpg`)
    )

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(files, conversationHistory, currentPendingFiles)

    // Should only accept MAX_IMAGES
    expect(result).toHaveLength(MAX_IMAGES)
    expect(result[0].filename).toBe('image0.jpg')
    expect(result[MAX_IMAGES - 1].filename).toBe(`image${MAX_IMAGES - 1}.jpg`)
  })

  it('should respect document count limit', () => {
    // Create more documents than the limit
    const files = Array.from({ length: MAX_DOCUMENTS + 3 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `doc${i}.pdf`)
    )

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(files, conversationHistory, currentPendingFiles)

    // Should only accept MAX_DOCUMENTS
    expect(result).toHaveLength(MAX_DOCUMENTS)
    expect(result[0].filename).toBe('doc0.pdf')
    expect(result[MAX_DOCUMENTS - 1].filename).toBe(`doc${MAX_DOCUMENTS - 1}.pdf`)
  })

  it('should respect document file size limit', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kPdf, MAX_DOCUMENT_SIZE_BYTES - 1024, 'small-doc.pdf'), // Under limit
      createMockFile(Mojom.UploadedFileType.kPdf, MAX_DOCUMENT_SIZE_BYTES, 'exact-size-doc.pdf'), // At limit
      createMockFile(Mojom.UploadedFileType.kPdf, MAX_DOCUMENT_SIZE_BYTES + 1024, 'large-doc.pdf'), // Over limit
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'another-small-doc.pdf') // Under limit
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(files, conversationHistory, currentPendingFiles)

    // Should only accept documents under or at the size limit
    expect(result).toHaveLength(3)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'small-doc.pdf' }),
        expect.objectContaining({ filename: 'exact-size-doc.pdf' }),
        expect.objectContaining({ filename: 'another-small-doc.pdf' })
      ])
    )

    // Should not include the large document
    expect(result).not.toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'large-doc.pdf' })
      ])
    )
  })

  it('should handle mixed file types with size limits', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, MAX_DOCUMENT_SIZE_BYTES + 1024, 'large-doc.pdf'), // Should be rejected
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'small-doc.pdf'), // Should be accepted
      createMockFile(Mojom.UploadedFileType.kScreenshot, 1024, 'screenshot.png')
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(files, conversationHistory, currentPendingFiles)

    // Should accept all files except the oversized document
    expect(result).toHaveLength(4)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'image1.jpg' }),
        expect.objectContaining({ filename: 'image2.jpg' }),
        expect.objectContaining({ filename: 'small-doc.pdf' }),
        expect.objectContaining({ filename: 'screenshot.png' })
      ])
    )

    // Should not include the large document
    expect(result).not.toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'large-doc.pdf' })
      ])
    )
  })



  it('should handle empty input arrays', () => {
    const files: Mojom.UploadedFile[] = []
    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(files, conversationHistory, currentPendingFiles)

    expect(result).toHaveLength(0)
  })

  it('should respect limits when current pending files exist', () => {
    // Create pending files
    const pendingImage1 = createMockFile(Mojom.UploadedFileType.kImage, 1024, 'pending-image1.jpg')
    const pendingImage2 = createMockFile(Mojom.UploadedFileType.kImage, 1024, 'pending-image2.jpg')
    const pendingDoc1 = createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'pending-doc1.pdf')

    const currentPendingFiles = [pendingImage1, pendingImage2, pendingDoc1]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc2.pdf')
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []

    const result = processUploadedFilesWithLimits(newFiles, conversationHistory, currentPendingFiles)

    // Should accept new files since we're under limits (2 pending + 2 new = 4 images, 1 pending + 2 new = 3 docs)
    expect(result).toHaveLength(4)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image1.jpg' }),
        expect.objectContaining({ filename: 'new-image2.jpg' }),
        expect.objectContaining({ filename: 'new-doc1.pdf' }),
        expect.objectContaining({ filename: 'new-doc2.pdf' })
      ])
    )
  })

  it('should respect limits when both conversation history and pending files exist', () => {
    // Create conversation history with existing files
    const existingImage1 = createMockFile(Mojom.UploadedFileType.kImage, 1024, 'existing-image1.jpg')
    const existingDoc1 = createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'existing-doc1.pdf')

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: [existingImage1, existingDoc1]
      }
    ]

    // Create pending files
    const pendingImage1 = createMockFile(Mojom.UploadedFileType.kImage, 1024, 'pending-image1.jpg')
    const pendingDoc1 = createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'pending-doc1.pdf')

    const currentPendingFiles = [pendingImage1, pendingDoc1]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf')
    ]

    const result = processUploadedFilesWithLimits(newFiles, conversationHistory, currentPendingFiles)

    // Should accept new files since we're under limits (1 existing + 1 pending + 1 new = 3 images, 1 existing + 1 pending + 1 new = 3 docs)
    expect(result).toHaveLength(2)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image1.jpg' }),
        expect.objectContaining({ filename: 'new-doc1.pdf' })
      ])
    )
  })

  it('should reject files when limits are exceeded', () => {
    // Create conversation history with many existing files
    const existingImages = Array.from({ length: 18 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, 1024, `existing-image${i}.jpg`)
    )
    const existingDocs = Array.from({ length: 3 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `existing-doc${i}.pdf`)
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: [...existingImages, ...existingDocs]
      }
    ]

    // Create pending files
    const pendingImages = Array.from({ length: 2 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, 1024, `pending-image${i}.jpg`)
    )
    const pendingDocs = Array.from({ length: 2 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `pending-doc${i}.pdf`)
    )

    const currentPendingFiles = [...pendingImages, ...pendingDocs]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image3.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc2.pdf')
    ]

    const result = processUploadedFilesWithLimits(newFiles, conversationHistory, currentPendingFiles)

    // Should only accept files that fit within limits:
    // Images: 18 existing + 2 pending = 20, so 0 new images allowed
    // Documents: 3 existing + 2 pending = 5, so 0 new docs allowed
    expect(result).toHaveLength(0)
  })

  it('should accept partial files when some limits are exceeded', () => {
    // Create conversation history with some existing files
    const existingImages = Array.from({ length: 15 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, 1024, `existing-image${i}.jpg`)
    )
    const existingDocs = Array.from({ length: 2 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `existing-doc${i}.pdf`)
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: [...existingImages, ...existingDocs]
      }
    ]

    // Create pending files
    const pendingImages = Array.from({ length: 3 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, 1024, `pending-image${i}.jpg`)
    )
    const pendingDocs = Array.from({ length: 2 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `pending-doc${i}.pdf`)
    )

    const currentPendingFiles = [...pendingImages, ...pendingDocs]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image3.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc2.pdf')
    ]

    const result = processUploadedFilesWithLimits(newFiles, conversationHistory, currentPendingFiles)

    // Should accept partial files:
    // Images: 15 existing + 3 pending = 18, so 2 new images allowed (20 - 18 = 2)
    // Documents: 2 existing + 2 pending = 4, so 1 new doc allowed (5 - 4 = 1)
    expect(result).toHaveLength(3)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image1.jpg' }),
        expect.objectContaining({ filename: 'new-image2.jpg' }),
        expect.objectContaining({ filename: 'new-doc1.pdf' })
      ])
    )

    // Should not include the files that exceed limits
    expect(result).not.toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image3.jpg' }),
        expect.objectContaining({ filename: 'new-doc2.pdf' })
      ])
    )
  })
})
