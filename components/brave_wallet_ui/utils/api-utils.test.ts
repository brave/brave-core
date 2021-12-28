import { GetTokenParam } from './api-utils'
import { mockNetworks } from '../stories/mock-data/mock-networks'
import { AccountAssetOptions } from '../options/asset-options'

describe('Check token param', () => {
  test('Value should return contract address', () => {
    expect(GetTokenParam(mockNetworks[0], AccountAssetOptions[1].asset)).toEqual('0x0D8775F648430679A709E98d2b0Cb6250d2887EF')
  })
  test('Value should return symbol', () => {
    expect(GetTokenParam(mockNetworks[1], AccountAssetOptions[1].asset)).toEqual('bat')
  })
})
