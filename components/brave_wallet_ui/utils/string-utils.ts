export const stripERC20TokenImageURL = (url?: string) =>
  url?.replace('chrome://erc-token-images/', '')

export const toProperCase = (value: string) =>
  value.replace(/\w\S*/g,
    (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase())

export const isRemoteImageURL = (url?: string) =>
  url?.startsWith('http://') || url?.startsWith('https://') || url?.startsWith('data:image/') || url?.startsWith('ipfs://')

export const isValidIconExtension = (url?: string) =>
  url?.endsWith('.jpg') || url?.endsWith('.jpeg') || url?.endsWith('.png') || url?.endsWith('.svg') || url?.endsWith('.gif')

export const httpifyIpfsUrl = (url: string | undefined) => {
  if (!url) {
    return ''
  }

  if (url.startsWith('data:image/')) {
    return url
  }

  return `chrome://image/?${url.includes('ipfs://') ? url.replace('ipfs://', 'https://ipfs.io/ipfs/') : url}`
}
