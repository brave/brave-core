/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { isImageFile } from './file_types'

describe('isImageFile', () => {
  // Helper function to create mock File objects
  const createMockFile = (name: string, type: string): File => {
    return new File([''], name, { type })
  }

  describe('supported image types', () => {
    it('accepts PNG files', () => {
      const file = createMockFile('test.png', 'image/png')
      expect(isImageFile(file)).toBe(true)
    })

    it('accepts JPEG files', () => {
      const file = createMockFile('test.jpeg', 'image/jpeg')
      expect(isImageFile(file)).toBe(true)
    })

    it('accepts JPG files', () => {
      const file = createMockFile('test.jpg', 'image/jpeg')
      expect(isImageFile(file)).toBe(true)
    })

    it('accepts WebP files', () => {
      const file = createMockFile('test.webp', 'image/webp')
      expect(isImageFile(file)).toBe(true)
    })
  })

  describe('unsupported file types', () => {
    it('rejects GIF files', () => {
      const file = createMockFile('test.gif', 'image/gif')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects SVG files', () => {
      const file = createMockFile('test.svg', 'image/svg+xml')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects BMP files', () => {
      const file = createMockFile('test.bmp', 'image/bmp')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects TIFF files', () => {
      const file = createMockFile('test.tiff', 'image/tiff')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects text files', () => {
      const file = createMockFile('test.txt', 'text/plain')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects PDF files', () => {
      const file = createMockFile('test.pdf', 'application/pdf')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects video files', () => {
      const file = createMockFile('test.mp4', 'video/mp4')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects audio files', () => {
      const file = createMockFile('test.mp3', 'audio/mpeg')
      expect(isImageFile(file)).toBe(false)
    })
  })

  describe('case sensitivity', () => {
    it('handles uppercase MIME types', () => {
      const file = createMockFile('test.png', 'IMAGE/PNG')
      expect(isImageFile(file)).toBe(true)
    })

    it('handles mixed case MIME types', () => {
      const file = createMockFile('test.jpeg', 'Image/JpEG')
      expect(isImageFile(file)).toBe(true)
    })

    it('handles uppercase rejected types', () => {
      const file = createMockFile('test.gif', 'IMAGE/GIF')
      expect(isImageFile(file)).toBe(false)
    })
  })

  describe('edge cases', () => {
    it('rejects empty MIME type', () => {
      const file = createMockFile('test', '')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects malformed MIME type', () => {
      const file = createMockFile('test', 'not-a-mime-type')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects partial match', () => {
      const file = createMockFile('test', 'image/png-extra')
      expect(isImageFile(file)).toBe(false)
    })

    it('rejects MIME type with extra spaces', () => {
      const file = createMockFile('test.png', ' image/png ')
      expect(isImageFile(file)).toBe(false)
    })
  })
})
