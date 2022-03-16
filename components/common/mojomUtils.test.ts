import { mojoTimeDeltaToJSDate } from './mojomUtils'

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
