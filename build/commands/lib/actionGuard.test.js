// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const fs = require('fs-extra')
const path = require('path')
const ActionGuard = require('./actionGuard')

describe('ActionGuard', () => {
  const guardFilePath = '/path/to/guard/file'
  const cleanupClosure = jest.fn()
  let files = {}

  beforeEach(() => {
    jest.clearAllMocks()
    files = {}

    fs.existsSync = jest.fn((filePath) => {
      return files.hasOwnProperty(filePath)
    })

    fs.readFileSync = jest.fn((filePath) => {
      if (!files.hasOwnProperty(filePath)) {
        throw new Error(`ENOENT: no such file or directory, open '${filePath}'`)
      }
      return files[filePath]
    })

    fs.writeFileSync = jest.fn((filePath, data) => {
      files[filePath] = data
    })

    fs.unlinkSync = jest.fn((filePath) => {
      delete files[filePath]
    })

    fs.ensureDirSync = jest.fn()
  })

  it('should simulate fs operations correctly', () => {
    // Simulate the existence of a file
    files['/path/to/file'] = 'File content'

    // Check if the file exists
    expect(fs.existsSync('/path/to/file')).toBe(true)
    expect(fs.existsSync('/path/to/nonexistent/file')).toBe(false)

    // Read the file
    const content = fs.readFileSync('/path/to/file')
    expect(content).toBe('File content')

    // Write to a file
    fs.writeFileSync('/path/to/new/file', 'New file content')
    expect(files['/path/to/new/file']).toBe('New file content')

    // Delete a file
    fs.unlinkSync('/path/to/file')
    expect(files.hasOwnProperty('/path/to/file')).toBe(false)
    expect(fs.existsSync('/path/to/file')).toBe(false)
  })

  describe('wasInterrupted', () => {
    it('should return false when the guard file does not exist', () => {
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      const result = actionGuard.wasInterrupted()
      expect(result).toBe(false)
      expect(fs.existsSync).toHaveBeenCalledWith(guardFilePath)
    })

    it('should return true when the guard file exists', () => {
      fs.writeFileSync(guardFilePath, '')
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      const result = actionGuard.wasInterrupted()
      expect(result).toBe(true)
      expect(fs.existsSync).toHaveBeenCalledWith(guardFilePath)
    })

    it('should throw an error if called while the action is running', () => {
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      actionGuard.run(() => {
        expect(() => {
          actionGuard.wasInterrupted()
        }).toThrow(
          'Cannot check if the action was interrupted while it is running.'
        )
      })
    })
  })

  describe('run', () => {
    it('should perform the action with a guard', () => {
      const actionClosure = jest.fn()
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      actionGuard.run(actionClosure)
      expect(fs.ensureDirSync).toHaveBeenCalledWith(path.dirname(guardFilePath))
      expect(fs.writeFileSync).toHaveBeenCalledWith(
        guardFilePath,
        expect.anything()
      )
      expect(actionClosure).toHaveBeenCalledWith(false)
      expect(fs.unlinkSync).toHaveBeenCalledWith(guardFilePath)
    })

    it('should allow another run after the action is completed', () => {
      const actionClosure = jest.fn()
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      actionGuard.run(actionClosure)
      expect(actionClosure).toHaveBeenCalledWith(false)
      actionGuard.run(actionClosure)
      expect(actionClosure).toHaveBeenCalledWith(false)
      expect(actionClosure).toHaveBeenCalledTimes(2)
    })

    it('should throw an error if called while running', () => {
      const actionClosure = jest.fn()
      const actionGuard = new ActionGuard(guardFilePath)
      expect(() => {
        actionGuard.run(() => {
          actionGuard.run(actionClosure)
        })
      }).toThrow('Cannot run the action while it is already running.')
    })

    it('should handle action interruption and cleanup correctly', () => {
      expect(!fs.existsSync(guardFilePath))

      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      expect(() => {
        actionGuard.run(() => {
          throw new Error('Action error')
        })
      }).toThrow()

      expect(fs.existsSync(guardFilePath))
      expect(cleanupClosure).not.toHaveBeenCalled()
      expect(actionGuard.wasInterrupted()).toBe(true)

      const actionClosure = jest.fn()
      actionGuard.run(actionClosure)
      expect(cleanupClosure).toHaveBeenCalled()
      expect(actionClosure).toHaveBeenCalledWith(true)
      expect(!fs.existsSync(guardFilePath))

      cleanupClosure.mockClear()
      actionClosure.mockClear()

      actionGuard.run(actionClosure)
      expect(cleanupClosure).not.toHaveBeenCalled()
      expect(actionClosure).toHaveBeenCalledWith(false)
      expect(!fs.existsSync(guardFilePath))
    })
  })

  describe('runWithCleanupClosure', () => {
    it('should perform the action if the guard file does not exist', () => {
      const actionClosure = jest.fn()
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      actionGuard.run(actionClosure)
      expect(fs.existsSync).toHaveBeenCalledWith(guardFilePath)
      expect(cleanupClosure).not.toHaveBeenCalled()
      expect(actionClosure).toHaveBeenCalledWith(false)
      expect(fs.writeFileSync).toHaveBeenCalledWith(
        guardFilePath,
        expect.anything()
      )
      expect(fs.unlinkSync).toHaveBeenCalledWith(guardFilePath)
    })

    it('should perform the cleanup and then perform the action if the guard file exists', () => {
      fs.writeFileSync(guardFilePath, '')
      const actionClosure = jest.fn()
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      actionGuard.run(actionClosure)
      expect(fs.existsSync).toHaveBeenCalledWith(guardFilePath)
      expect(cleanupClosure).toHaveBeenCalled()
      expect(actionClosure).toHaveBeenCalledWith(true)
      expect(fs.writeFileSync).toHaveBeenCalledWith(
        guardFilePath,
        expect.anything()
      )
      expect(fs.unlinkSync).toHaveBeenCalledWith(guardFilePath)
    })

    it('should throw an error if the cleanup removes the guard file', () => {
      fs.writeFileSync(guardFilePath, '')
      const actionClosure = jest.fn()
      const actionGuard = new ActionGuard(guardFilePath, () => {
        fs.unlinkSync(guardFilePath)
      })
      expect(() => {
        actionGuard.run(actionClosure)
      }).toThrow(/Cleanup should not remove the guard/)
    })
  })
})
