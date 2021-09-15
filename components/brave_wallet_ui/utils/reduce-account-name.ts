export const reduceAccountDisplayName = (name: string, maxLength: number) => {
  if (!name) {
    return ''
  } else {
    if (name.length > maxLength) {
      const sliced = name.slice(0, maxLength - 2)
      const reduced = sliced.concat('..')
      return reduced
    } else {
      return name
    }
  }
}
