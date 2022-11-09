// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { copyToClipboard } from './copy-to-clipboard'

Object.assign(navigator, {
  clipboard: {
    writeText: (text: string) => {}
  }
})

describe('copyToClipboard', () => {
  it('should call navigator.clipboard.writeText', async (done) => {
    const mockData = 'someText'
    jest.spyOn(navigator.clipboard, 'writeText')
    await copyToClipboard(mockData)
    expect(navigator.clipboard.writeText).toHaveBeenCalledWith(mockData)

    done()
  })

  it('should throw error if navigator.clipboard.writeText throws an error', async (done) => {
    jest.spyOn(navigator.clipboard, 'writeText').mockImplementation(() => {
      throw new Error()
    })
    await copyToClipboard('data')
    expect(navigator.clipboard.writeText).toThrowError()

    done()
  })
})
