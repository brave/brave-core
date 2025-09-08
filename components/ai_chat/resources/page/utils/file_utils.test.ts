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
import getAPI from '../api'

// Mock the API module
jest.mock('../api', () => ({
  __esModule: true,
  default: jest.fn(),
}))

// Mock loadTimeData (required by API initialization)
jest.mock('$web-common/loadTimeData', () => ({
  loadTimeData: {
    getBoolean: jest.fn().mockReturnValue(false),
  },
}))

describe('convertFileToUploadedFile', () => {
  let mockHandler: any
  let mockService: any
  let mockAPI: any
  let mockFileReader: any
  let originalFileReader: any

  beforeEach(() => {
    setupMojomServiceMocks()
    setupAPIMock()
    setupFileReaderMock()
  })

  afterEach(() => {
    global.FileReader = originalFileReader
    jest.clearAllMocks()
  })

  function setupMojomServiceMocks() {
    // Mock the UI handler that processes images
    mockHandler = {
      processImageFile: jest.fn(),
    }

    // Mock the main service for PageAPI initialization
    mockService = {
      bindObserver: jest.fn().mockResolvedValue({ state: {} }),
      getConversations: jest.fn().mockResolvedValue({ conversations: [] }),
      getActionMenuList: jest.fn().mockResolvedValue({ actionList: [] }),
      getPremiumStatus: jest.fn().mockResolvedValue({ status: undefined }),
      bindMetrics: jest.fn(),
    }

    // Mock the getRemote functions
    jest.spyOn(Mojom.Service, 'getRemote').mockReturnValue(mockService)
    jest.spyOn(Mojom.AIChatUIHandler, 'getRemote').mockReturnValue(mockHandler)
  }

  function setupAPIMock() {
    // Mock the complete API instance that getAPI() returns
    mockAPI = {
      uiHandler: mockHandler,
      service: mockService,
      setPartialState: jest.fn(),
      initialize: jest.fn(),
      updateCurrentPremiumStatus: jest.fn(),
    }
    ;(getAPI as jest.Mock).mockReturnValue(mockAPI)
  }

  function setupFileReaderMock() {
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
  }

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
      }

      mockHandler.processImageFile.mockResolvedValue({
        processedFile: mockProcessedFile,
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      const result = await convertFileToUploadedFile(file)

      expect(mockFileReader.readAsArrayBuffer).toHaveBeenCalledWith(file)
      expect(mockHandler.processImageFile).toHaveBeenCalledWith(
        expectedData,
        'test.png',
      )
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
      }

      mockHandler.processImageFile.mockResolvedValue({
        processedFile: mockProcessedFile,
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      const result = await convertFileToUploadedFile(file)

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

      const result = await convertFileToUploadedFile(file)

      expect(mockFileReader.readAsArrayBuffer).toHaveBeenCalledWith(file)
      expect(mockHandler.processImageFile).not.toHaveBeenCalled()
      expect(result).toEqual({
        filename: 'test.pdf',
        filesize: file.size, // Use actual file size
        data: expectedData,
        type: Mojom.UploadedFileType.kPdf,
      })
    })

    it('throws ImageProcessingError for empty files', async () => {
      const file = createMockFile('empty.png', 'image/png', 0)
      const mockArrayBuffer = new ArrayBuffer(0)

      // Mock processImageFile to return null for empty files (real behavior)
      mockHandler.processImageFile.mockResolvedValue({
        processedFile: null,
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        ImageProcessingError,
      )
      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        'Failed to process image file: Backend returned no result',
      )
      expect(mockHandler.processImageFile).toHaveBeenCalledWith([], 'empty.png')
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

      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        FileReadError,
      )
      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        'Failed to read file: test.png',
      )
      expect(mockHandler.processImageFile).not.toHaveBeenCalled()
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

      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        FileReadError,
      )
      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        'Failed to read file: No data received',
      )
      expect(mockHandler.processImageFile).not.toHaveBeenCalled()
    })

    it('throws ImageProcessingError when processImageFile returns null', async () => {
      const file = createMockFile('test.png', 'image/png')
      const mockArrayBuffer = new ArrayBuffer(8)

      mockHandler.processImageFile.mockResolvedValue({
        processedFile: null,
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        ImageProcessingError,
      )
      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
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

      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        UnsupportedFileTypeError,
      )
      await expect(convertFileToUploadedFile(file)).rejects.toThrow(
        'Unsupported file type: text/plain. Only images and PDF files are '
          + 'supported.',
      )
      expect(mockHandler.processImageFile).not.toHaveBeenCalled()
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

      mockHandler.processImageFile.mockResolvedValue({
        processedFile: {
          filename: 'data.png',
          filesize: 1000,
          data: expectedData,
          type: Mojom.UploadedFileType.kImage,
        },
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await convertFileToUploadedFile(file)

      expect(mockHandler.processImageFile).toHaveBeenCalledWith(
        expectedData,
        'data.png',
      )
    })

    it('preserves original filename', async () => {
      const filename = 'my special file with spaces.png'
      const file = createMockFile(filename, 'image/png')
      const mockArrayBuffer = new ArrayBuffer(8)

      mockHandler.processImageFile.mockResolvedValue({
        processedFile: {
          filename: filename,
          filesize: 1000,
          data: [1, 2, 3],
          type: Mojom.UploadedFileType.kImage,
        },
      })

      // Override readAsArrayBuffer to trigger success
      mockFileReader.readAsArrayBuffer.mockImplementation(() => {
        process.nextTick(() => {
          if (mockFileReader.onload) {
            mockFileReader.onload({ target: { result: mockArrayBuffer } })
          }
        })
      })

      await convertFileToUploadedFile(file)

      expect(mockHandler.processImageFile).toHaveBeenCalledWith(
        expect.any(Array),
        filename,
      )
    })
  })
})
