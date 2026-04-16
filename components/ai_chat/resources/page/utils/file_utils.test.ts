/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  convertFileToUploadedFile,
  FileReadError,
  ImageProcessingError,
  UnsupportedFileTypeError,
} from './file_utils'
import * as Mojom from '../../common/mojom'
import type { AIChatContext } from '../state/ai_chat_context'

describe('convertFileToUploadedFile', () => {
  let mockProcessImageFile: jest.Mock<
    ReturnType<AIChatContext['processImageFile']>,
    Parameters<AIChatContext['processImageFile']>
  >
  let mockFileReader: any
  let originalFileReader: any

  beforeEach(() => {
    // Mock processImageFile function (now passed as parameter)
    mockProcessImageFile = jest.fn()

    // Mock FileReader for simulating file reading
    originalFileReader = global.FileReader
    mockFileReader = {
      readAsArrayBuffer: jest.fn(),
      onload: null,
      onerror: null,
      result: null,
    }

    // Mock the FileReader constructor to return our mock
    global.FileReader = jest
      .fn()
      .mockImplementation(() => mockFileReader) as any
  })

  afterEach(() => {
    global.FileReader = originalFileReader
    jest.clearAllMocks()
  })

  // Test helpers
  const createMockFile = (
    name: string,
    type: string,
    size: number = 1000,
  ): File => {
    return new File(['mock content'], name, { type, size: size })
  }

  describe('successful file processing', () => {
    it('converts image file successfully', async () => {
      const file = createMockFile('test.png', 'image/png', 1500)
      const mockArrayBuffer = new ArrayBuffer(8)
      const expectedData = Array.from(new Uint8Array(mockArrayBuffer))
      const mockProcessedFile = {
        filename: 'processed_test.png',
        filesize: 1200,
        data: [1, 2, 3, 4],
        type: Mojom.UploadedFileType.kImage,
        extractedText: undefined,
      }

      mockProcessImageFile.mockResolvedValue(mockProcessedFile)

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      const result = await convertFileToUploadedFile(file, mockProcessImageFile)

      expect(mockFileReader.readAsArrayBuffer).toHaveBeenCalledWith(file)
      expect(mockProcessImageFile).toHaveBeenCalledWith([
        expectedData,
        'test.png',
      ])
      expect(result).toEqual(mockProcessedFile)
    })

    it('handles different file types', async () => {
      const file = createMockFile('screenshot.jpeg', 'image/jpeg')
      const mockArrayBuffer = new ArrayBuffer(16)
      const mockProcessedFile = {
        filename: 'screenshot.jpeg',
        filesize: 1000,
        data: [5, 6, 7, 8],
        type: Mojom.UploadedFileType.kScreenshot,
        extractedText: undefined,
      }

      mockProcessImageFile.mockResolvedValue(mockProcessedFile)

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      const result = await convertFileToUploadedFile(file, mockProcessImageFile)

      expect(result).toEqual(mockProcessedFile)
    })

    it('converts PDF file successfully', async () => {
      const file = createMockFile('test.pdf', 'application/pdf', 2000)
      const mockArrayBuffer = new ArrayBuffer(8)
      const expectedData = Array.from(new Uint8Array(mockArrayBuffer))

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      const result = await convertFileToUploadedFile(file, mockProcessImageFile)

      expect(mockFileReader.readAsArrayBuffer).toHaveBeenCalledWith(file)
      expect(mockProcessImageFile).not.toHaveBeenCalled()
      expect(result).toEqual({
        filename: 'test.pdf',
        filesize: file.size, // Use actual file size
        data: expectedData,
        type: Mojom.UploadedFileType.kPdf,
        extractedText: undefined,
      })
    })

    it('throws ImageProcessingError for empty files', async () => {
      const file = createMockFile('empty.png', 'image/png', 0)
      const mockArrayBuffer = new ArrayBuffer(0)

      // Mock processImageFile to return null for empty files (real behavior)
      mockProcessImageFile.mockResolvedValue(
        null as unknown as Mojom.UploadedFile,
      )

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(ImageProcessingError)
      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(
        'Failed to process image file: Backend returned no result',
      )
      expect(mockProcessImageFile).toHaveBeenCalledWith([[], 'empty.png'])
    })
  })

  describe('error handling', () => {
    it('throws FileReadError when FileReader fails to read file', async () => {
      const file = createMockFile('test.png', 'image/png')

      // Override readAsArrayBuffer to trigger error immediately
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onerror) {
            mockFileReader.onerror()
          }
        })
      })

      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(FileReadError)
      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow('Failed to read file: test.png')
      expect(mockProcessImageFile).not.toHaveBeenCalled()
    })

    it('throws FileReadError when FileReader result is null', async () => {
      const file = createMockFile('test.png', 'image/png')

      // Override readAsArrayBuffer to trigger onload with null result
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: null } })
          }
        })
      })

      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(FileReadError)
      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow('Failed to read file: No data received')
      expect(mockProcessImageFile).not.toHaveBeenCalled()
    })

    it('throws ImageProcessingError when processImageFile returns null', async () => {
      const file = createMockFile('test.png', 'image/png')
      const mockArrayBuffer = new ArrayBuffer(8)

      mockProcessImageFile.mockResolvedValue(
        null as unknown as Mojom.UploadedFile,
      )

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(ImageProcessingError)
      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(
        'Failed to process image file: Backend returned no result',
      )
    })

    it('throws UnsupportedFileTypeError for unknown file types', async () => {
      const file = createMockFile('test.txt', 'text/plain')
      const mockArrayBuffer = new ArrayBuffer(8)

      // Override readAsArrayBuffer to trigger success
      // (FileReader will work, but file type check will fail)
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(UnsupportedFileTypeError)
      await expect(
        convertFileToUploadedFile(file, mockProcessImageFile),
      ).rejects.toThrow(
        'Unsupported file type: text/plain. Only images and PDF files are '
          + 'supported.',
      )
      expect(mockProcessImageFile).not.toHaveBeenCalled()
    })
  })

  describe('data transformation', () => {
    it('correctly transforms ArrayBuffer to Uint8Array', async () => {
      const file = createMockFile('data.png', 'image/png')
      // Create specific byte pattern
      const mockArrayBuffer = new ArrayBuffer(4)
      const view = new Uint8Array(mockArrayBuffer)
      view[0] = 255
      view[1] = 128
      view[2] = 64
      view[3] = 32
      const expectedData = [255, 128, 64, 32]

      mockProcessImageFile.mockResolvedValue({
        filename: 'data.png',
        filesize: 1000,
        data: expectedData,
        type: Mojom.UploadedFileType.kImage,
        extractedText: undefined,
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await convertFileToUploadedFile(file, mockProcessImageFile)

      expect(mockProcessImageFile).toHaveBeenCalledWith([
        expectedData,
        'data.png',
      ])
    })

    it('preserves original filename', async () => {
      const filename = 'my special file with spaces.png'
      const file = createMockFile(filename, 'image/png')
      const mockArrayBuffer = new ArrayBuffer(8)

      mockProcessImageFile.mockResolvedValue({
        filename: filename,
        filesize: 1000,
        data: [1, 2, 3],
        type: Mojom.UploadedFileType.kImage,
        extractedText: undefined,
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await convertFileToUploadedFile(file, mockProcessImageFile)

      expect(mockProcessImageFile).toHaveBeenCalledWith([
        expect.any(Array),
        filename,
      ])
    })
  })
})
