export const copyToClipboard = async (data: string) => {
  try {
    await navigator.clipboard.writeText(data)
  } catch (e) {
    console.log(`Could not copy address ${e.toString()}`)
  }
}
