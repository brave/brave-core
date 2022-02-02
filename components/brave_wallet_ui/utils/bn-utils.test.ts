import {
  addNumericValues,
  multiplyNumericValues,
  normalizeNumericValue,
  isNumericValueGreaterThan
} from './bn-utils'

describe('bn-utils', () => {
  it('should add two numeric values', () => {
    expect(addNumericValues('0x1', '0x2')).toEqual('3')
    expect(addNumericValues('1', '2')).toEqual('3')
  })

  it('should multiply two numeric values', () => {
    expect(multiplyNumericValues('0x2', '0x3')).toEqual('6')
    expect(multiplyNumericValues('2', '3')).toEqual('6')
  })

  it('should normalize a hex numeric value', () => {
    expect(normalizeNumericValue('0x2')).toEqual('2')
  })

  it('should normalize invalid numeric values to empty string', () => {
    expect(normalizeNumericValue('')).toEqual('')
    expect(normalizeNumericValue('0x')).toEqual('')
    expect(normalizeNumericValue('invalid value')).toEqual('')
  })

  it('should compare two numeric values', () => {
    expect(isNumericValueGreaterThan('0x2', '0x3')).toEqual(false)
    expect(isNumericValueGreaterThan('2', '3')).toEqual(false)
    expect(isNumericValueGreaterThan('3', '2')).toEqual(true)
  })
})
