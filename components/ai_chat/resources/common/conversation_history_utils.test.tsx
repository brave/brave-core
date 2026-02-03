// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  processUploadedFilesWithLimits,
  getImageFiles,
  getDocumentFiles,
  shouldDisableAttachmentsButton,
  isFullPageScreenshot,
  formatConversationForClipboard,
} from './conversation_history_utils'
import * as Mojom from './mojom'
import {
  createConversationTurnWithDefaults,
  getCompletionEvent,
  getWebSourcesEvent,
} from './test_data_utils'

describe('processUploadedFilesWithLimits', () => {
  const createMockFile = (
    type: Mojom.UploadedFileType,
    filesize: number,
    filename: string = 'test-file',
  ): Mojom.UploadedFile => ({
    type,
    filesize: filesize,
    filename,
    data: [1, 2, 3, 4],
  })

  it('should process files in original order while respecting limits', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'doc2.pdf'),
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(
      files,
      conversationHistory,
      currentPendingFiles,
    )

    // Should accept all files since we're under limits
    expect(result).toHaveLength(4)
    expect(result[0].filename).toBe('image1.jpg')
    expect(result[1].filename).toBe('doc1.pdf')
    expect(result[2].filename).toBe('image2.jpg')
    expect(result[3].filename).toBe('doc2.pdf')
  })

  it('should respect image count limit', () => {
    // Create more images than the limit
    const files = Array.from({ length: Mojom.MAX_IMAGES + 5 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, 1024, `image${i}.jpg`),
    )

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(
      files,
      conversationHistory,
      currentPendingFiles,
    )

    // Should only accept Mojom.MAX_IMAGES
    expect(result).toHaveLength(Mojom.MAX_IMAGES)
    expect(result[0].filename).toBe('image0.jpg')
    expect(result[Mojom.MAX_IMAGES - 1].filename).toBe(
      `image${Mojom.MAX_IMAGES - 1}.jpg`,
    )
  })

  it('should respect document count limit', () => {
    // Create more documents than the limit
    const files = Array.from({ length: Mojom.MAX_DOCUMENTS + 3 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `doc${i}.pdf`),
    )

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(
      files,
      conversationHistory,
      currentPendingFiles,
    )

    // Should only accept Mojom.MAX_DOCUMENTS
    expect(result).toHaveLength(Mojom.MAX_DOCUMENTS)
    expect(result[0].filename).toBe('doc0.pdf')
    expect(result[Mojom.MAX_DOCUMENTS - 1].filename).toBe(
      `doc${Mojom.MAX_DOCUMENTS - 1}.pdf`,
    )
  })

  it('should respect document file size limit', () => {
    const files = [
      createMockFile(
        Mojom.UploadedFileType.kPdf,
        Mojom.MAX_DOCUMENT_SIZE_BYTES - 1024,
        'small-doc.pdf',
      ), // Under limit
      createMockFile(
        Mojom.UploadedFileType.kPdf,
        Mojom.MAX_DOCUMENT_SIZE_BYTES,
        'exact-size-doc.pdf',
      ), // At limit
      createMockFile(
        Mojom.UploadedFileType.kPdf,
        Mojom.MAX_DOCUMENT_SIZE_BYTES + 1024,
        'large-doc.pdf',
      ), // Over limit
      createMockFile(
        Mojom.UploadedFileType.kPdf,
        1024,
        'another-small-doc.pdf',
      ), // Under limit
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(
      files,
      conversationHistory,
      currentPendingFiles,
    )

    // Should only accept documents under or at the size limit
    expect(result).toHaveLength(3)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'small-doc.pdf' }),
        expect.objectContaining({ filename: 'exact-size-doc.pdf' }),
        expect.objectContaining({ filename: 'another-small-doc.pdf' }),
      ]),
    )

    // Should not include the large document
    expect(result).not.toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'large-doc.pdf' }),
      ]),
    )
  })

  it('should handle mixed file types with size limits', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image1.jpg'),
      createMockFile(
        Mojom.UploadedFileType.kPdf,
        Mojom.MAX_DOCUMENT_SIZE_BYTES + 1024,
        'large-doc.pdf',
      ), // Should be rejected
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'small-doc.pdf'), // Should be accepted
      createMockFile(
        Mojom.UploadedFileType.kScreenshot,
        1024,
        'screenshot.png',
      ),
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(
      files,
      conversationHistory,
      currentPendingFiles,
    )

    // Should accept all files except the oversized document
    expect(result).toHaveLength(4)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'image1.jpg' }),
        expect.objectContaining({ filename: 'image2.jpg' }),
        expect.objectContaining({ filename: 'small-doc.pdf' }),
        expect.objectContaining({ filename: 'screenshot.png' }),
      ]),
    )

    // Should not include the large document
    expect(result).not.toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'large-doc.pdf' }),
      ]),
    )
  })

  it('should handle empty input arrays', () => {
    const files: Mojom.UploadedFile[] = []
    const conversationHistory: Mojom.ConversationTurn[] = []
    const currentPendingFiles: Mojom.UploadedFile[] = []

    const result = processUploadedFilesWithLimits(
      files,
      conversationHistory,
      currentPendingFiles,
    )

    expect(result).toHaveLength(0)
  })

  it('should respect limits when current pending files exist', () => {
    // Create pending files
    const pendingImage1 = createMockFile(
      Mojom.UploadedFileType.kImage,
      1024,
      'pending-image1.jpg',
    )
    const pendingImage2 = createMockFile(
      Mojom.UploadedFileType.kImage,
      1024,
      'pending-image2.jpg',
    )
    const pendingDoc1 = createMockFile(
      Mojom.UploadedFileType.kPdf,
      1024,
      'pending-doc1.pdf',
    )

    const currentPendingFiles = [pendingImage1, pendingImage2, pendingDoc1]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc2.pdf'),
    ]

    const conversationHistory: Mojom.ConversationTurn[] = []

    const result = processUploadedFilesWithLimits(
      newFiles,
      conversationHistory,
      currentPendingFiles,
    )

    // Should accept new files since we're under limits (2 pending + 2 new = 4 images, 1 pending + 2 new = 3 docs)
    expect(result).toHaveLength(4)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image1.jpg' }),
        expect.objectContaining({ filename: 'new-image2.jpg' }),
        expect.objectContaining({ filename: 'new-doc1.pdf' }),
        expect.objectContaining({ filename: 'new-doc2.pdf' }),
      ]),
    )
  })

  it('should respect limits when both conversation history and pending files exist', () => {
    // Create conversation history with existing files
    const existingImage1 = createMockFile(
      Mojom.UploadedFileType.kImage,
      1024,
      'existing-image1.jpg',
    )
    const existingDoc1 = createMockFile(
      Mojom.UploadedFileType.kPdf,
      1024,
      'existing-doc1.pdf',
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: [existingImage1, existingDoc1],
      },
    ]

    // Create pending files
    const pendingImage1 = createMockFile(
      Mojom.UploadedFileType.kImage,
      1024,
      'pending-image1.jpg',
    )
    const pendingDoc1 = createMockFile(
      Mojom.UploadedFileType.kPdf,
      1024,
      'pending-doc1.pdf',
    )

    const currentPendingFiles = [pendingImage1, pendingDoc1]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf'),
    ]

    const result = processUploadedFilesWithLimits(
      newFiles,
      conversationHistory,
      currentPendingFiles,
    )

    // Should accept new files since we're under limits (1 existing + 1 pending + 1 new = 3 images, 1 existing + 1 pending + 1 new = 3 docs)
    expect(result).toHaveLength(2)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image1.jpg' }),
        expect.objectContaining({ filename: 'new-doc1.pdf' }),
      ]),
    )
  })

  it('should reject files when limits are exceeded', () => {
    // Create conversation history with many existing files
    const existingImages = Array.from({ length: 18 }, (_, i) =>
      createMockFile(
        Mojom.UploadedFileType.kImage,
        1024,
        `existing-image${i}.jpg`,
      ),
    )
    const existingDocs = Array.from({ length: 3 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `existing-doc${i}.pdf`),
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: [...existingImages, ...existingDocs],
      },
    ]

    // Create pending files
    const pendingImages = Array.from({ length: 2 }, (_, i) =>
      createMockFile(
        Mojom.UploadedFileType.kImage,
        1024,
        `pending-image${i}.jpg`,
      ),
    )
    const pendingDocs = Array.from({ length: 2 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `pending-doc${i}.pdf`),
    )

    const currentPendingFiles = [...pendingImages, ...pendingDocs]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image3.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc2.pdf'),
    ]

    const result = processUploadedFilesWithLimits(
      newFiles,
      conversationHistory,
      currentPendingFiles,
    )

    // Should only accept files that fit within limits:
    // Images: 18 existing + 2 pending = 20, so 0 new images allowed
    // Documents: 3 existing + 2 pending = 5, so 0 new docs allowed
    expect(result).toHaveLength(0)
  })

  it('should accept partial files when some limits are exceeded', () => {
    // Create conversation history with some existing files
    const existingImages = Array.from({ length: 15 }, (_, i) =>
      createMockFile(
        Mojom.UploadedFileType.kImage,
        1024,
        `existing-image${i}.jpg`,
      ),
    )
    const existingDocs = Array.from({ length: 2 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `existing-doc${i}.pdf`),
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: [...existingImages, ...existingDocs],
      },
    ]

    // Create pending files
    const pendingImages = Array.from({ length: 3 }, (_, i) =>
      createMockFile(
        Mojom.UploadedFileType.kImage,
        1024,
        `pending-image${i}.jpg`,
      ),
    )
    const pendingDocs = Array.from({ length: 2 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, `pending-doc${i}.pdf`),
    )

    const currentPendingFiles = [...pendingImages, ...pendingDocs]

    // Try to upload more files
    const newFiles = [
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image1.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image2.jpg'),
      createMockFile(Mojom.UploadedFileType.kImage, 1024, 'new-image3.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc1.pdf'),
      createMockFile(Mojom.UploadedFileType.kPdf, 1024, 'new-doc2.pdf'),
    ]

    const result = processUploadedFilesWithLimits(
      newFiles,
      conversationHistory,
      currentPendingFiles,
    )

    // Should accept partial files:
    // Images: 15 existing + 3 pending = 18, so 2 new images allowed (20 - 18 = 2)
    // Documents: 2 existing + 2 pending = 4, so 1 new doc allowed (5 - 4 = 1)
    expect(result).toHaveLength(3)
    expect(result).toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image1.jpg' }),
        expect.objectContaining({ filename: 'new-image2.jpg' }),
        expect.objectContaining({ filename: 'new-doc1.pdf' }),
      ]),
    )

    // Should not include the files that exceed limits
    expect(result).not.toEqual(
      expect.arrayContaining([
        expect.objectContaining({ filename: 'new-image3.jpg' }),
        expect.objectContaining({ filename: 'new-doc2.pdf' }),
      ]),
    )
  })
})

describe('getImageFiles', () => {
  const createMockFile = (
    type: Mojom.UploadedFileType,
    filename: string = 'test-file',
  ): Mojom.UploadedFile => ({
    type,
    filesize: 1024,
    filename,
    data: [1, 2, 3, 4],
  })

  it('should filter out only image and screenshot files', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kImage, 'image.jpg'),
      createMockFile(Mojom.UploadedFileType.kScreenshot, 'screenshot.png'),
      createMockFile(Mojom.UploadedFileType.kPdf, 'document.pdf'),
    ]

    const result = getImageFiles(files)

    expect(result).toHaveLength(2)
    expect(result).toEqual([
      expect.objectContaining({ filename: 'image.jpg' }),
      expect.objectContaining({ filename: 'screenshot.png' }),
    ])
  })

  it('should return undefined when no files are provided', () => {
    expect(getImageFiles(undefined)).toBeUndefined()
  })

  it('should return empty array when no image files exist', () => {
    const files = [createMockFile(Mojom.UploadedFileType.kPdf, 'document.pdf')]

    const result = getImageFiles(files)

    expect(result).toHaveLength(0)
  })

  it('should return empty array for empty input', () => {
    const result = getImageFiles([])

    expect(result).toHaveLength(0)
  })
})

describe('getDocumentFiles', () => {
  const createMockFile = (
    type: Mojom.UploadedFileType,
    filename: string = 'test-file',
  ): Mojom.UploadedFile => ({
    type,
    filesize: 1024,
    filename,
    data: [1, 2, 3, 4],
  })

  it('should filter out only PDF document files', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kImage, 'image.jpg'),
      createMockFile(Mojom.UploadedFileType.kPdf, 'document1.pdf'),
      createMockFile(Mojom.UploadedFileType.kScreenshot, 'screenshot.png'),
      createMockFile(Mojom.UploadedFileType.kPdf, 'document2.pdf'),
    ]

    const result = getDocumentFiles(files)

    expect(result).toHaveLength(2)
    expect(result).toEqual([
      expect.objectContaining({ filename: 'document1.pdf' }),
      expect.objectContaining({ filename: 'document2.pdf' }),
    ])
  })

  it('should return undefined when no files are provided', () => {
    expect(getDocumentFiles(undefined)).toBeUndefined()
  })

  it('should return empty array when no document files exist', () => {
    const files = [
      createMockFile(Mojom.UploadedFileType.kImage, 'image.jpg'),
      createMockFile(Mojom.UploadedFileType.kScreenshot, 'screenshot.png'),
    ]

    const result = getDocumentFiles(files)

    expect(result).toHaveLength(0)
  })

  it('should return empty array for empty input', () => {
    const result = getDocumentFiles([])

    expect(result).toHaveLength(0)
  })
})

describe('shouldDisableAttachmentsButton', () => {
  const createMockFile = (
    type: Mojom.UploadedFileType,
    filename: string = 'test-file',
  ): Mojom.UploadedFile => ({
    type,
    filesize: 1024,
    filename,
    data: [1, 2, 3, 4],
  })

  it('should return false when conversation history is empty', () => {
    const conversationHistory: Mojom.ConversationTurn[] = []

    expect(shouldDisableAttachmentsButton(conversationHistory)).toBe(false)
  })

  it('should return false when under both limits', () => {
    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: [
          createMockFile(Mojom.UploadedFileType.kImage, 'image1.jpg'),
          createMockFile(Mojom.UploadedFileType.kPdf, 'doc1.pdf'),
          createMockFile(Mojom.UploadedFileType.kScreenshot, 'screenshot1.png'),
        ],
      },
    ]

    expect(shouldDisableAttachmentsButton(conversationHistory)).toBe(false)
  })

  it('should return true when image limit is reached', () => {
    const images = Array.from({ length: Mojom.MAX_IMAGES }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, `image${i}.jpg`),
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: images,
      },
    ]

    expect(shouldDisableAttachmentsButton(conversationHistory)).toBe(true)
  })

  it('should return true when document limit is reached', () => {
    const documents = Array.from({ length: Mojom.MAX_DOCUMENTS }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kPdf, `doc${i}.pdf`),
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: documents,
      },
    ]

    expect(shouldDisableAttachmentsButton(conversationHistory)).toBe(true)
  })

  it('should return true when either limit is exceeded', () => {
    const images = Array.from({ length: Mojom.MAX_IMAGES + 1 }, (_, i) =>
      createMockFile(Mojom.UploadedFileType.kImage, `image${i}.jpg`),
    )

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test',
        uploadedFiles: images,
      },
    ]

    expect(shouldDisableAttachmentsButton(conversationHistory)).toBe(true)
  })

  it('should count files across multiple conversation turns', () => {
    const imagesPerTurn = Math.floor(Mojom.MAX_IMAGES / 2) + 1

    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test1',
        uploadedFiles: Array.from({ length: imagesPerTurn }, (_, i) =>
          createMockFile(Mojom.UploadedFileType.kImage, `image1-${i}.jpg`),
        ),
      },
      {
        uuid: 'turn2',
        text: 'test2',
        uploadedFiles: Array.from({ length: imagesPerTurn }, (_, i) =>
          createMockFile(Mojom.UploadedFileType.kImage, `image2-${i}.jpg`),
        ),
      },
    ]

    expect(shouldDisableAttachmentsButton(conversationHistory)).toBe(true)
  })

  it('should handle conversation turns with no uploaded files', () => {
    const conversationHistory = [
      {
        uuid: 'turn1',
        text: 'test1',
        uploadedFiles: [
          createMockFile(Mojom.UploadedFileType.kImage, 'image1.jpg'),
        ],
      },
      {
        uuid: 'turn2',
        text: 'test2',
        // no uploadedFiles property
      },
    ]

    expect(shouldDisableAttachmentsButton(conversationHistory)).toBe(false)
  })
})

describe('isFullPageScreenshot', () => {
  const createMockFile = (
    filename: string,
    type: Mojom.UploadedFileType,
  ): Mojom.UploadedFile => ({
    filename,
    type,
    data: new ArrayBuffer(100),
    filesize: BigInt(100),
  })

  it('returns true for full page screenshots', () => {
    const file = createMockFile(
      `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
      Mojom.UploadedFileType.kScreenshot,
    )
    expect(isFullPageScreenshot(file)).toBe(true)
  })

  it('returns false for regular screenshots without prefix', () => {
    const file = createMockFile(
      'regular_screenshot.png',
      Mojom.UploadedFileType.kScreenshot,
    )
    expect(isFullPageScreenshot(file)).toBe(false)
  })

  it('returns false for non-screenshot files with screenshot-like names', () => {
    const file = createMockFile(
      `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
      Mojom.UploadedFileType.kImage,
    )
    expect(isFullPageScreenshot(file)).toBe(false)
  })

  it('returns false for PDF files', () => {
    const file = createMockFile(
      `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}0.png`,
      Mojom.UploadedFileType.kPdf,
    )
    expect(isFullPageScreenshot(file)).toBe(false)
  })

  it('handles files with different screenshot indices', () => {
    const file1 = createMockFile(
      `${Mojom.FULL_PAGE_SCREENSHOT_PREFIX}123.png`,
      Mojom.UploadedFileType.kScreenshot,
    )
    expect(isFullPageScreenshot(file1)).toBe(true)

    const file2 = createMockFile(
      'screenshot_123.png',
      Mojom.UploadedFileType.kScreenshot,
    )
    expect(isFullPageScreenshot(file2)).toBe(false)
  })
})

describe('formatConversationForClipboard', () => {
  beforeEach(() => {
    global.S = {
      CHAT_UI_COPY_LABEL_YOU: 'You',
    }
  })

  it('should format a single human message', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: 'Hello, how are you?',
        characterType: Mojom.CharacterType.HUMAN,
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe('You: Hello, how are you?')
  })

  it('should format a single assistant message using text field', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: 'I am doing well, thank you!',
        characterType: Mojom.CharacterType.ASSISTANT,
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe('Leo AI: I am doing well, thank you!')
  })

  it(
    'should use completion event text for assistant messages when'
      + 'available',
    () => {
      const conversationHistory = [
        createConversationTurnWithDefaults({
          uuid: 'turn1',
          text: 'fallback text',
          characterType: Mojom.CharacterType.ASSISTANT,
          events: [getCompletionEvent('This is the completion text')],
        }),
      ]

      const result = formatConversationForClipboard(conversationHistory)
      expect(result).toBe('Leo AI: This is the completion text')
    },
  )

  it('should format a full conversation with multiple turns', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: 'What is the capital of France?',
        characterType: Mojom.CharacterType.HUMAN,
      }),
      createConversationTurnWithDefaults({
        uuid: 'turn2',
        text: '',
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [getCompletionEvent('The capital of France is Paris.')],
      }),
      createConversationTurnWithDefaults({
        uuid: 'turn3',
        text: 'Tell me more about it.',
        characterType: Mojom.CharacterType.HUMAN,
      }),
      createConversationTurnWithDefaults({
        uuid: 'turn4',
        text: '',
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getCompletionEvent(
            'Paris is the largest city in France and a major cultural center.',
          ),
        ],
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe(
      'You: What is the capital of France?\n\n'
        + 'Leo AI: The capital of France is Paris.\n\n'
        + 'You: Tell me more about it.\n\n'
        + 'Leo AI: Paris is the largest city in France and a major '
        + 'cultural center.',
    )
  })

  it('should return empty string for empty conversation history', () => {
    const result = formatConversationForClipboard([])
    expect(result).toBe('')
  })

  it('should fall back to text field when no completion event exists', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: 'Fallback assistant text',
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [],
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe('Leo AI: Fallback assistant text')
  })

  it('should replace citation numbers with URLs in assistant messages', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: '',
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getWebSourcesEvent([
            {
              url: { url: 'https://example.com/article1' },
              title: 'Article 1',
              faviconUrl: { url: 'https://example.com/favicon.ico' },
            },
            {
              url: { url: 'https://example.com/article2' },
              title: 'Article 2',
              faviconUrl: { url: 'https://example.com/favicon.ico' },
            },
          ]),
          getCompletionEvent(
            'This is a response with citations [1] and [2] in the text.',
          ),
        ],
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe(
      'Leo AI: This is a response with citations https://example.com/article1 and https://example.com/article2 in the text.',
    )
  })

  it('should add space before URL when citation follows a word', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: '',
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getWebSourcesEvent([
            {
              url: { url: 'https://example.com/article1' },
              title: 'Article 1',
              faviconUrl: { url: 'https://example.com/favicon.ico' },
            },
          ]),
          getCompletionEvent(
            'This is text without space[1] and with space [1].',
          ),
        ],
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe(
      'Leo AI: This is text without space https://example.com/article1 and '
        + 'with space https://example.com/article1.',
    )
  })

  it('keeps citation number when link is missing for that index', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: '',
        characterType: Mojom.CharacterType.ASSISTANT,
        events: [
          getWebSourcesEvent([
            {
              url: { url: 'https://example.com/article1' },
              title: 'Article 1',
              faviconUrl: { url: 'https://example.com/favicon.ico' },
            },
          ]),
          getCompletionEvent(
            'This has valid citation [1] and invalid [3] citations.',
          ),
        ],
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe(
      'Leo AI: This has valid citation https://example.com/article1 and invalid [3] citations.',
    )
  })

  it('should not replace citations in human messages', () => {
    const conversationHistory = [
      createConversationTurnWithDefaults({
        uuid: 'turn1',
        text: 'Check out reference [1]',
        characterType: Mojom.CharacterType.HUMAN,
      }),
    ]

    const result = formatConversationForClipboard(conversationHistory)
    expect(result).toBe('You: Check out reference [1]')
  })
})
