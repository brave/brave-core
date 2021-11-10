import { mojoTimeDeltaToJSDate, formatDateAsRelative } from './datetime-utils'

describe('Convertting from MojoTime works', () => {
  test('0 epoch time value', () => {
    // Epoch time
    expect(mojoTimeDeltaToJSDate({ microseconds: 0 })).toEqual(new Date(null))
  })
  test('converts normal values ok', () => {
    const milliseconds = 1635363887716
    const microseconds = milliseconds * 1000
    const expectedDate = new Date()
    expectedDate.setTime(milliseconds)
    expect(mojoTimeDeltaToJSDate({ microseconds })).toEqual(expectedDate)
  })
})

describe('Relative dates format correctly', () => {
  const now = new Date(2021, 2, 1)
  test('seconds', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 5)  // 5S
    expect(formatDateAsRelative(date, now)).toEqual('5s')
  })

  test('minutes', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 2)  // 2M
    expect(formatDateAsRelative(date, now)).toEqual('2m')
  })

  test('hours', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 2)  // 2H
    expect(formatDateAsRelative(date, now)).toEqual('2h')
  })

  test('days', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 24 * 3)  // 3D
    expect(formatDateAsRelative(date, now)).toEqual('3d')
  })

  test('same year - month day', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 24 * 25)  // 25 Days
    expect(formatDateAsRelative(date, now)).toEqual('Feb 4')
  })

  test('diff year - month day year', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 24 * 60)  // 90 Days
    expect(formatDateAsRelative(date, now)).toEqual('Dec 31, 2020')
  })
})
