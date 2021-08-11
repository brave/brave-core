export const formatPrices = (total: number) => {
  const fixed = total.toFixed(2)
  const parts = fixed.toString().split('.')
  return (
    parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, ',') +
    (parts[1] ? '.' + parts[1] : '')
  )
}
