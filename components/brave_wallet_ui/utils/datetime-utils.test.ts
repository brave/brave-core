import { formatDateAsRelative } from './datetime-utils'

describe('Relative dates format correctly', () => {
  const now = new Date(2021, 2, 1)
  test('seconds', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 5) // 5S
    expect(formatDateAsRelative(date, now)).toEqual('5s')
  })

  test('minutes', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 2) // 2M
    expect(formatDateAsRelative(date, now)).toEqual('2m')
  })

  test('hours', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 2) // 2H
    expect(formatDateAsRelative(date, now)).toEqual('2h')
  })

  test('days', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 24 * 3) // 3D
    expect(formatDateAsRelative(date, now)).toEqual('3d')
  })

  test('same year - month day', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 24 * 25) // 25 Days
    expect(formatDateAsRelative(date, now)).toEqual('Feb 4')
  })

  test('diff year - month day year', () => {
    const date = new Date(now)
    date.setTime(date.getTime() - 1000 * 60 * 60 * 24 * 60) // 90 Days
    expect(formatDateAsRelative(date, now)).toEqual('Dec 31, 2020')
  })
})
