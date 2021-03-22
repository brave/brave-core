export const reduceAddress = (address: string) => {
  const firstHalf = address.slice(0, 6)
  const secondHalf = address.slice(-4)
  const reduced = firstHalf.concat('***', secondHalf)
  return reduced
}
