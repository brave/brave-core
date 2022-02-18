import { GetNetworkInfo, emptyNetwork, reduceNetworkDisplayName } from './network-utils'
import { mockNetworks } from '../stories/mock-data/mock-networks'

describe('getNetworkInfo', () => {
  it('should return network info', () => {
    const chainId = mockNetworks[0].chainId
    expect(GetNetworkInfo(chainId, mockNetworks)).toEqual(mockNetworks[0])
  })

  it('should return network object with default values if network with chainId is not found', () => {
    const chainId = 'fakeChainId'
    expect(GetNetworkInfo(chainId, mockNetworks)).toEqual(emptyNetwork)
  })
})

describe('reduceNetworkDisplayName', () => {
  it('should return empty string if network name is falsy', () => {
    expect(reduceNetworkDisplayName('')).toEqual('')
  })

  it('should return first word when network name contains spaces', () => {
    const result = reduceNetworkDisplayName('Ethereum Mainet')
    expect(result).toEqual('Ethereum')
  })

  it('should concatenate first six character of network name and ..', () => {
    const networkName = 'TestNetworkName'
    const expected = 'TestNe..'
    expect(reduceNetworkDisplayName(networkName)).toBe(expected)
  })
})
