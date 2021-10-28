import { isStrongPassword } from './password-utils'

describe('Check if password is strong', () => {
  test('Password meets all requirements except including a number.', () => {
    const value = 'BraveRocks!'
    expect(isStrongPassword.test(value)).toEqual(false)
  })

  test('Password meets all requirements except including a special character.', () => {
    const value = 'BraveRocks1'
    expect(isStrongPassword.test(value)).toEqual(false)
  })

  test('Password meets all requirements except being 7 characters long.', () => {
    const value = 'Brav#1'
    expect(isStrongPassword.test(value)).toEqual(false)
  })

  test('Password meets all requirements.', () => {
    const value = 'Brave#1'
    expect(isStrongPassword.test(value)).toEqual(true)
  })
})
