// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { pickFiles } from './uploadFile'

describe('pickFiles', () => {
  beforeEach(() => {
    jest.spyOn(HTMLInputElement.prototype, 'click').mockImplementation(() => {})
  })

  function getLastInput (): HTMLInputElement {
    const inputs = document.querySelectorAll<HTMLInputElement>('input[type="file"]')
    return inputs[inputs.length - 1]
  }

  it('appends a file input to document.body and calls click', async () => {
    const promise = pickFiles()
    const input = getLastInput()
    expect(input).toBeTruthy()
    expect(input.type).toBe('file')
    expect(document.body.contains(input)).toBe(true)
    expect(HTMLInputElement.prototype.click).toHaveBeenCalledTimes(1)
    input.dispatchEvent(new Event('cancel'))
    await promise
  })

  it('resolves with an empty array when the dialog is cancelled', async () => {
    const promise = pickFiles()
    getLastInput().dispatchEvent(new Event('cancel'))
    expect(await promise).toEqual([])
  })

  it('resolves with an empty array when change fires with no files', async () => {
    const promise = pickFiles()
    getLastInput().dispatchEvent(new Event('change'))
    expect(await promise).toEqual([])
  })

  it('resolves with the selected files when change fires', async () => {
    const promise = pickFiles()
    const input = getLastInput()
    const file = new File(['hello'], 'hello.txt', { type: 'text/plain' })
    Object.defineProperty(input, 'files', { value: [file], configurable: true })
    input.dispatchEvent(new Event('change'))
    const result = await promise
    expect(result).toHaveLength(1)
    expect(result[0].name).toBe('hello.txt')
    expect(result[0].type).toBe('text/plain')
  })

  it('resolves with multiple files when multiple are selected', async () => {
    const promise = pickFiles({ multiple: true })
    const input = getLastInput()
    const files = [
      new File(['a'], 'a.txt', { type: 'text/plain' }),
      new File(['b'], 'b.txt', { type: 'text/plain' })
    ]
    Object.defineProperty(input, 'files', { value: files, configurable: true })
    input.dispatchEvent(new Event('change'))
    const result = await promise
    expect(result).toHaveLength(2)
    expect(result[0].name).toBe('a.txt')
    expect(result[1].name).toBe('b.txt')
  })

  it('removes the input from the document after settling', async () => {
    const promise = pickFiles()
    const input = getLastInput()
    expect(document.body.contains(input)).toBe(true)
    input.dispatchEvent(new Event('cancel'))
    await promise
    expect(document.body.contains(input)).toBe(false)
  })

  it('only settles once even if both cancel and change fire', async () => {
    const promise = pickFiles()
    const input = getLastInput()
    input.dispatchEvent(new Event('cancel'))
    const file = new File(['x'], 'x.txt')
    Object.defineProperty(input, 'files', { value: [file], configurable: true })
    input.dispatchEvent(new Event('change'))
    expect(await promise).toEqual([])
  })

  it('sets multiple to false by default', async () => {
    const promise = pickFiles()
    expect(getLastInput().multiple).toBe(false)
    getLastInput().dispatchEvent(new Event('cancel'))
    await promise
  })

  it('sets multiple to true when specified', async () => {
    const promise = pickFiles({ multiple: true })
    expect(getLastInput().multiple).toBe(true)
    getLastInput().dispatchEvent(new Event('cancel'))
    await promise
  })

  it('sets the accept attribute when specified', async () => {
    const promise = pickFiles({ accept: 'image/*' })
    expect(getLastInput().accept).toBe('image/*')
    getLastInput().dispatchEvent(new Event('cancel'))
    await promise
  })

  it('does not set the accept attribute when unspecified', async () => {
    const promise = pickFiles()
    expect(getLastInput().accept).toBe('')
    getLastInput().dispatchEvent(new Event('cancel'))
    await promise
  })

  it('sets the capture attribute when specified', async () => {
    const promise = pickFiles({ capture: 'environment' })
    expect(getLastInput().getAttribute('capture')).toBe('environment')
    getLastInput().dispatchEvent(new Event('cancel'))
    await promise
  })

  it('does not set the capture attribute when unspecified', async () => {
    const promise = pickFiles()
    expect(getLastInput().getAttribute('capture')).toBeNull()
    getLastInput().dispatchEvent(new Event('cancel'))
    await promise
  })
})
