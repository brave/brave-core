export const stripERC20TokenImageURL = (url?: string) =>
  url?.replace('chrome://erc-token-images/', '')

export const toProperCase = (value: string) =>
  value.replace(/\w\S*/g,
    (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase())
