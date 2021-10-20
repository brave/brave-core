const addCommas = (value: string) => {
  const parts = value.split('.')
  return (
    parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ',') +
    (parts[1] ? '.' + parts[1] : '')
  )
}

export const formatWithCommasAndDecimals = (value: string) => {
  const valueToNumber = Number(value)

  // Handle case of empty string as well as zero-value.
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
  const added = Number(calculatedDecimalPlace) + 4
  return addCommas(valueToNumber.toFixed(added))
}
