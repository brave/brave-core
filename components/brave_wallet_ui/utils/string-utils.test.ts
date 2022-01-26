import { isRemoteImageURL, isValidIconExtension } from './string-utils'

describe('Checking URL is remote image or not', () => {
  test('HTTP URL should return true', () => {
    expect(isRemoteImageURL('http://test.com/test.png')).toEqual(true)
  })

  test('HTTPS URL should return true', () => {
    expect(isRemoteImageURL('https://test.com/test.png')).toEqual(true)
  })

  test('Data URL image should return true', () => {
    expect(isRemoteImageURL('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR42mP4z8AAAAMBAQD3A0FDAAAAAElFTkSuQmCC')).toEqual(true)
  })

  test('local path should return false', () => {
    expect(isRemoteImageURL('bat.png')).toEqual(false)
  })

  test('undefined input should return undefined', () => {
    expect(isRemoteImageURL(undefined)).toEqual(undefined)
  })
})

describe('Checking URL ends with a valid icon extension', () => {
  test('Ends with .png should return true', () => {
    expect(isValidIconExtension('http://test.com/test.png')).toEqual(true)
  })

  test('Ends with .svg should return true', () => {
    expect(isValidIconExtension('https://test.com/test.svg')).toEqual(true)
  })

  test('Ends with .jpg should return true', () => {
    expect(isValidIconExtension('https://test.com/test.jpg')).toEqual(true)
  })

  test('Ends with .jpeg should return true', () => {
    expect(isValidIconExtension('https://test.com/test.jpeg')).toEqual(true)
  })

  test('Ends with .com should return false', () => {
    expect(isValidIconExtension('https://test.com/')).toEqual(false)
  })
})
