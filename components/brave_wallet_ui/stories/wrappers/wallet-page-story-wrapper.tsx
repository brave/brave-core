import * as React from 'react'
import { createStore, combineReducers } from 'redux'
import { Provider } from 'react-redux'
import { MemoryRouter } from 'react-router-dom'

// utils
import { createSendCryptoReducer } from '../../common/reducers/send_crypto_reducer'
import { createWalletReducer } from '../../common/reducers/wallet_reducer'
import { createPageReducer } from '../../page/reducers/page_reducer'

// actions
import { WalletActions } from '../../common/actions'

// types
import { PageState, WalletState } from '../../constants/types'

// components
import { LibContext } from '../../common/context/lib.context'

// Mocks
import * as Lib from '../../common/async/__mocks__/lib'
import { mockPageState } from '../mock-data/mock-page-state'
import { mockWalletState } from '../mock-data/mock-wallet-state'
import { mockSendCryptoState } from '../mock-data/send-crypto-state'
import { ApiProxyContext } from '../../common/context/api-proxy.context'
import { getMockedAPIProxy } from '../../common/async/__mocks__/bridge'

export interface WalletPageStoryProps {
  walletStateOverride?: Partial<WalletState>
  pageStateOverride?: Partial<PageState>
}

const mockedProxy = getMockedAPIProxy()

export const WalletPageStory: React.FC<React.PropsWithChildren<WalletPageStoryProps>> = ({
  children,
  pageStateOverride,
  walletStateOverride
}) => {
  // redux
  const store = React.useMemo(() => {
    return createStore(combineReducers({
      wallet: createWalletReducer({
        ...mockWalletState,
        ...(walletStateOverride || {})
      }),
      page: createPageReducer({
        ...mockPageState,
        ...(pageStateOverride || {})
      }),
      sendCrypto: createSendCryptoReducer(mockSendCryptoState)
    }))
  }, [walletStateOverride, pageStateOverride])

  React.useEffect(() => {
    store && store.dispatch(WalletActions.initialize())
  }, [store])

  // render
  return (
    <MemoryRouter initialEntries={['/']}>
      <Provider store={store}>
        <ApiProxyContext.Provider value={mockedProxy}>
          <LibContext.Provider value={Lib as any}>
            {children}
          </LibContext.Provider>
        </ApiProxyContext.Provider>
      </Provider>
    </MemoryRouter>
  )
}

export default WalletPageStory
