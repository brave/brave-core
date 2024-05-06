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

  beforeEach(() => {
    jest.clearAllMocks()
    fs.existsSync = jest.fn().mockReturnValue(false)
    fs.writeFileSync = jest.fn()
    fs.ensureDirSync = jest.fn()
    fs.unlinkSync = jest.fn()
  })

  describe('wasInterrupted', () => {
    it('should return false when the guard file does not exist', () => {
      const actionGuard = new ActionGuard(guardFilePath, cleanupClosure)
      const result = actionGuard.wasInterrupted()
      expect(result).toBe(false)
      expect(fs.existsSync).toHaveBeenCalledWith(guardFilePath)
    })

    it('should return true when the guard file exists', () => {
      fs.existsSync = jest.fn().mockReturnValue(true)
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

    it('should set isRunning to false after the action is completed', () => {
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
      fs.existsSync = jest.fn().mockReturnValue(true)
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
      fs.existsSync = jest.fn().mockReturnValue(true)
      fs.unlinkSync = jest.fn().mockImplementation(() => {
        fs.existsSync = jest.fn().mockReturnValue(false)
      })
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
