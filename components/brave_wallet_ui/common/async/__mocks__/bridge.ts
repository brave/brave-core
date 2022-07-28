import { BraveWallet } from '../../../constants/types'
import { SwapParamsPayloadType } from '../../constants/action_types'
import WalletApiProxy from '../../wallet_api_proxy'
export class MockedWalletApiProxy {
  mockQuote = {
    price: '1705.399509',
    guaranteedPrice: '',
    to: '',
    data: '',
    value: '124067000000000000',
    gas: '280000',
    estimatedGas: '280000',
    gasPrice: '2000000000',
    protocolFee: '0',
    minimumProtocolFee: '0',
    sellTokenAddress: '0x07865c6e87b9f70255377e024ace6630c1eaa37f',
    buyTokenAddress: '0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee',
    buyAmount: '211599920',
    sellAmount: '124067000000000000',
    allowanceTarget: '0x0000000000000000000000000000000000000000',
    sellTokenToEthRate: '1',
    buyTokenToEthRate: '1720.180416'
  }

  mockTransaction = {
    allowanceTarget: '',
    price: '',
    guaranteedPrice: '',
    to: '',
    data: '',
    value: '',
    gas: '0',
    estimatedGas: '0',
    gasPrice: '0',
    protocolFee: '0',
    minimumProtocolFee: '0',
    buyTokenAddress: '',
    sellTokenAddress: '',
    buyAmount: '0',
    sellAmount: '0',
    sellTokenToEthRate: '1',
    buyTokenToEthRate: '1'
  }

  swapService = {
    getTransactionPayload: async ({
      fromAsset,
      toAsset,
      fromAssetAmount,
      toAssetAmount
    }: SwapParamsPayloadType): Promise<{ success: boolean, errorResponse: any, response: BraveWallet.SwapResponse }> => ({
      success: true,
      errorResponse: {},
      response: {
        ...this.mockQuote,
        buyTokenAddress: toAsset.contractAddress,
        sellTokenAddress: fromAsset.contractAddress,
        buyAmount: toAssetAmount || '',
        sellAmount: fromAssetAmount || '',
        price: '1'
      }
      // as BraveWallet.SwapResponse
    }),
    getPriceQuote: async () => ({
      success: true,
      errorResponse: {},
      response: this.mockTransaction
    })
  }

  keyringService = {
    validatePassword: async () => ({ result: true })
  }

  ethTxManagerProxy = {
    getGasEstimation1559: async () => {
      return {
        estimation: {
          slowMaxPriorityFeePerGas: '0',
          slowMaxFeePerGas: '0',
          avgMaxPriorityFeePerGas: '0',
          avgMaxFeePerGas: '0',
          fastMaxPriorityFeePerGas: '0',
          fastMaxFeePerGas: '0',
          baseFeePerGas: '0'
        } as BraveWallet.GasEstimation1559 | null
      }
    }
  }

  setMockedQuote (newQuote: typeof this.mockQuote) {
    this.mockQuote = newQuote
  }

  setMockedTransactionPayload (newTx: typeof this.mockQuote) {
    this.mockTransaction = newTx
  }
}

export function getAPIProxy (): Partial<WalletApiProxy> {
  return new MockedWalletApiProxy() as unknown as Partial<WalletApiProxy> & MockedWalletApiProxy
}

export function getMockedAPIProxy (): WalletApiProxy {
  return new MockedWalletApiProxy() as unknown as WalletApiProxy
}

export default getAPIProxy
