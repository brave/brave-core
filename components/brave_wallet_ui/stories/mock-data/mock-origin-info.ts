import { BraveWallet } from '../../constants/types'

export const mockOriginInfo: BraveWallet.OriginInfo = {
  origin: {
    scheme: 'https',
    host: 'with_a_really_looooooong_site_name.fixme.uniswap.org',
    port: 443,
    nonceIfOpaque: undefined
  },
  originSpec: 'https://with_a_really_looooooong_site_name.fixme.uniswap.org',
  eTldPlusOne: 'uniswap.org'
}
