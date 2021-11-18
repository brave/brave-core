const addCommas = (value: string) => {
  const parts = value.split('.')
  return (
    parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ',') +
    (parts[1] ? '.' + parts[1] : '')
  )
}

export const formatWithCommasAndDecimals = (value: string) => {
  // empty string indicates unknown balance
  if (value === '') {
    return ''
  }

  const valueToNumber = Number(value)

  if (valueToNumber === 0) {
    return '0.00'
  }

  if (valueToNumber >= 10) {
    return addCommas(valueToNumber.toFixed(2))
  }

  if (valueToNumber >= 1) {
    return addCommas(valueToNumber.toFixed(3))
  }

  const calculatedDecimalPlace = -Math.floor(Math.log(valueToNumber) / Math.log(10) + 1)
  const added = Number(calculatedDecimalPlace) + 3
  return addCommas(valueToNumber.toFixed(added))
}

export const formatFiatAmountWithCommasAndDecimals = (value: string): string => {
  if (!value) {
    return ''
  }
  // Check to make sure a formated value is returned before showing the fiat symbol
  if (!formatWithCommasAndDecimals(value)) {
    return ''
  }
  return '$' + formatWithCommasAndDecimals(value)
}

export const formatTokenAmountWithCommasAndDecimals = (value: string, symbol: string): string => {
  // Empty string indicates unknown balance
  if (!value && !symbol) {
    return ''
  }
  // Check to make sure a formated value is returned before showing the symbol
  if (!formatWithCommasAndDecimals(value)) {
    return ''
  }
  return formatWithCommasAndDecimals(value) + ' ' + symbol
}
