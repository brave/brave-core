import * as React from 'react'

// Utils
import { getUniqueAssets } from '../../../utils/asset-utils'
import {
  UserAccountType,
  BuySendSwapViewTypes,
  BraveWallet,
  DefaultCurrencies
} from '../../../constants/types'

// Hooks
import { useAssets } from '../../../common/hooks'

// Components
import {
  AccountsAssetsNetworks,
  Header,
  Buy
} from '..'

export interface Props {
  showHeader?: boolean
  defaultCurrencies: DefaultCurrencies
  onSelectAccount: (account: UserAccountType) => void
}

function BuyTab (props: Props) {
  const {
    showHeader,
    onSelectAccount
  } = props
  // Custom Hooks
  const { buyAssetOptions } = useAssets()
  const [buyView, setBuyView] = React.useState<BuySendSwapViewTypes>('buy')
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken>(buyAssetOptions[0])

  const onChangeBuyView = (view: BuySendSwapViewTypes) => {
    setBuyView(view)
  }

  const onClickSelectAccount = (account: UserAccountType) => () => {
    onSelectAccount(account)
    setBuyView('buy')
  }

  const onSelectedAsset = (asset: BraveWallet.BlockchainToken) => () => {
    setSelectedAsset(asset)
    setBuyView('buy')
  }

  const onSelectCurrency = React.useCallback(() => {
    // hide currency selection view
    setBuyView('buy')
  }, [setBuyView])

  const goBack = () => {
    setBuyView('buy')
  }

  // Memos
  const filteredAssetOptions = React.useMemo(() => {
    return getUniqueAssets(buyAssetOptions)
  }, [buyAssetOptions])

  React.useEffect(() => {
    if (buyAssetOptions.length > 0) {
      setSelectedAsset(buyAssetOptions[0])
    }
  }, [buyAssetOptions])

  return (
    <>
      {buyView === 'buy' &&
        <>
          {showHeader &&
            <Header
              onChangeSwapView={onChangeBuyView}
            />
          }
          <Buy
            selectedAsset={selectedAsset}
            onChangeBuyView={onChangeBuyView}
          />
        </>
      }
      {buyView !== 'buy' &&
        <AccountsAssetsNetworks
          goBack={goBack}
          assetOptions={filteredAssetOptions}
          onClickSelectAccount={onClickSelectAccount}
          onSelectedAsset={onSelectedAsset}
          onSelectCurrency={onSelectCurrency}
          selectedView={buyView}
        />
      }
    </>
  )
}

export default BuyTab
