/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { processFiles } from './file_utils'
import * as Mojom from '../../common/mojom'

// jsdom doesn't implement File.prototype.arrayBuffer or .text — polyfill via FileReader.
beforeAll(() => {
  if (!File.prototype.arrayBuffer) {
    File.prototype.arrayBuffer = function () {
      return new Promise<ArrayBuffer>((resolve, reject) => {
        const reader = new FileReader()
        reader.onload = () => resolve(reader.result as ArrayBuffer)
        reader.onerror = () => reject(reader.error)
        reader.readAsArrayBuffer(this)
      })
    }
  }
  if (!File.prototype.text) {
    File.prototype.text = function () {
      return new Promise<string>((resolve, reject) => {
        const reader = new FileReader()
        reader.onload = () => resolve(reader.result as string)
        reader.onerror = () => reject(reader.error)
        reader.readAsText(this)
      })
    }
  }
})

describe('processFiles', () => {
  let mockAttachFiles: jest.Mock

  beforeEach(() => {
    mockAttachFiles = jest.fn()
  })

  const createMockFile = (
    name: string,
    type: string,
    content: string = 'mock content',
  ): File => {
    return new File([content], name, { type })
  }

  it('converts image file to kImage type', async () => {
    const file = createMockFile('test.png', 'image/png')
    await processFiles([file], mockAttachFiles)

    expect(mockAttachFiles).toHaveBeenCalledWith([
      expect.objectContaining({
        filename: 'test.png',
        type: Mojom.UploadedFileType.kImage,
        extractedText: undefined,
      }),
    ])
  })

  it('converts PDF file to kPdf type', async () => {
    const file = createMockFile('test.pdf', 'application/pdf')
    await processFiles([file], mockAttachFiles)

    expect(mockAttachFiles).toHaveBeenCalledWith([
      expect.objectContaining({
        filename: 'test.pdf',
        type: Mojom.UploadedFileType.kPdf,
        extractedText: undefined,
      }),
    ])
  })

  it('converts text file to kText type with extracted text', async () => {
    const file = createMockFile('test.txt', 'text/plain', 'hello world')
    await processFiles([file], mockAttachFiles)

    expect(mockAttachFiles).toHaveBeenCalledWith([
      expect.objectContaining({
        filename: 'test.txt',
        type: Mojom.UploadedFileType.kText,
        extractedText: 'hello world',
      }),
    ])
  })

  it('includes byte data for all file types', async () => {
    const file = createMockFile('test.png', 'image/png', 'abc')
    await processFiles([file], mockAttachFiles)

    const [uploadedFiles] = mockAttachFiles.mock.calls[0]
    expect(uploadedFiles[0].data).toEqual(expect.any(Array))
    expect(uploadedFiles[0].data.length).toBeGreaterThan(0)
  })

  it('preserves file size', async () => {
    const file = createMockFile('test.pdf', 'application/pdf', 'x'.repeat(500))
    await processFiles([file], mockAttachFiles)

    const [uploadedFiles] = mockAttachFiles.mock.calls[0]
    expect(uploadedFiles[0].filesize).toBe(file.size)
  })

  it('converts multiple files and calls attachFiles once', async () => {
    const files = [
      createMockFile('image.png', 'image/png'),
      createMockFile('doc.pdf', 'application/pdf'),
      createMockFile('notes.txt', 'text/plain', 'some text'),
    ]
    await processFiles(files, mockAttachFiles)

    expect(mockAttachFiles).toHaveBeenCalledTimes(1)
    expect(mockAttachFiles).toHaveBeenCalledWith([
      expect.objectContaining({
        filename: 'image.png',
        type: Mojom.UploadedFileType.kImage,
      }),
      expect.objectContaining({
        filename: 'doc.pdf',
        type: Mojom.UploadedFileType.kPdf,
      }),
      expect.objectContaining({
        filename: 'notes.txt',
        type: Mojom.UploadedFileType.kText,
      }),
    ])
  })

  it('treats unknown MIME types as kText', async () => {
    const file = createMockFile('data.bin', 'application/octet-stream')
    await processFiles([file], mockAttachFiles)

    expect(mockAttachFiles).toHaveBeenCalledWith([
      expect.objectContaining({ type: Mojom.UploadedFileType.kText }),
    ])
  })

  it('does not include extractedText for image files', async () => {
    const file = createMockFile('img.jpeg', 'image/jpeg')
    await processFiles([file], mockAttachFiles)

    const [uploadedFiles] = mockAttachFiles.mock.calls[0]
    expect(uploadedFiles[0].extractedText).toBeUndefined()
  })

  it('does not include extractedText for PDF files', async () => {
    const file = createMockFile('doc.pdf', 'application/pdf')
    await processFiles([file], mockAttachFiles)

    const [uploadedFiles] = mockAttachFiles.mock.calls[0]
    expect(uploadedFiles[0].extractedText).toBeUndefined()
  })
})
