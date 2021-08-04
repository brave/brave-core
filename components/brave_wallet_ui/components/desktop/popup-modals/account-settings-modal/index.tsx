import * as React from 'react'
import * as qr from 'qr-image'
import {
  AccountSettingsNavTypes,
  WalletAccountType,
  TokenInfo,
  AccountAssetOptionType
} from '../../../../constants/types'
import {
  PopupModal,
  TopTabNav,
  AssetWatchlistItem
} from '../..'
import { AccountSettingsNavOptions } from '../../../../options/account-settings-nav-options'
import { reduceAddress } from '../../../../utils/reduce-address'
import { NavButton } from '../../../extension'
import { Tooltip, SearchBar } from '../../../shared'
import locale from '../../../../constants/locale'

// Styled Components
import {
  Input,
  StyledWrapper,
  QRCodeWrapper,
  AddressButton,
  ButtonRow,
  CopyIcon,
  WatchlistScrollContainer,
  LoadIcon,
  LoadingWrapper
} from './style'

export interface Props {
  onClose: () => void
  onUpdateAccountName?: (name: string) => void
  onUpdateVisibleTokens: (list: string[]) => void
  onCopyToClipboard?: () => void
  onChangeTab?: (id: AccountSettingsNavTypes) => void
  hideNav: boolean
  fullAssetList: TokenInfo[]
  userAssetList: AccountAssetOptionType[]
  userWatchList: string[]
  tab: AccountSettingsNavTypes
  title: string
  account?: WalletAccountType
}

const AddAccountModal = (props: Props) => {
  const {
    title,
    account,
    tab,
    userWatchList,
    fullAssetList,
    userAssetList,
    hideNav,
    onClose,
    onUpdateAccountName,
    onUpdateVisibleTokens,
    onCopyToClipboard,
    onChangeTab
  } = props
  const watchlist = React.useMemo(() => {
    const visibleList = userAssetList.map((item) => { return item.asset })
    const notVisibleList = fullAssetList.filter((item) => !userWatchList.includes(item.contractAddress))
    return [...visibleList, ...notVisibleList]
  }, [fullAssetList, userWatchList, userAssetList])
  const name = account ? account.name : ''
  const [accountName, setAccountName] = React.useState<string>(name)
  const [filteredWatchlist, setFilteredWatchlist] = React.useState<TokenInfo[]>([])
  const [selectedWatchlist, setSelectedWatchlist] = React.useState<string[]>(userWatchList)
  const [qrCode, setQRCode] = React.useState<string>('')

  React.useMemo(() => {
    setFilteredWatchlist(watchlist)
  }, [watchlist])

  const filterWatchlist = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setFilteredWatchlist(watchlist)
    } else {
      const filteredList = watchlist.filter((item) => {
        return (
          item.name.toLowerCase() === search.toLowerCase() ||
          item.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.symbol.toLowerCase().startsWith(search.toLowerCase()) ||
          item.contractAddress.toLocaleLowerCase() === search.toLowerCase()
        )
      })
      setFilteredWatchlist(filteredList)
    }
  }

  const onClickUpdateVisibleTokens = () => {
    onUpdateVisibleTokens(selectedWatchlist)
    onClose()
  }

  const onCheckWatchlistItem = (key: string, selected: boolean) => {
    if (selected) {
      const newList = [...selectedWatchlist, key]
      setSelectedWatchlist(newList)
    } else {
      const newList = selectedWatchlist.filter((id) => id !== key)
      setSelectedWatchlist(newList)
    }
  }

  const isAssetSelected = (key: string) => {
    return selectedWatchlist.includes(key)
  }

  const handleAccountNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setAccountName(event.target.value)
  }

  const onSubmitUpdateName = () => {
    if (onUpdateAccountName) {
      onUpdateAccountName(accountName)
    }
  }

  const generateQRData = () => {
    const address = account ? account.address : ''
    const image = qr.image(address)
    let chunks: Array<Uint8Array> = []
    image
      .on('data', (chunk: Uint8Array) => chunks.push(chunk))
      .on('end', () => {
        setQRCode(`data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`)
      })
  }

  React.useEffect(() => {
    generateQRData()
  })

  const changeTab = (id: AccountSettingsNavTypes) => {
    if (onChangeTab !== undefined) {
      onChangeTab(id)
    }
  }

  return (
    <PopupModal title={title} onClose={onClose}>
      {!hideNav &&
        <TopTabNav
          tabList={AccountSettingsNavOptions}
          onSubmit={changeTab}
          selectedTab={tab}
        />
      }
      <StyledWrapper>
        {tab === 'details' &&
          <>
            <Input
              value={accountName}
              placeholder={locale.addAccountPlaceholder}
              onChange={handleAccountNameChanged}
            />
            <QRCodeWrapper src={qrCode} />
            <Tooltip text={locale.toolTipCopyToClipboard}>
              <AddressButton onClick={onCopyToClipboard}>{reduceAddress(account ? account.address : '')}<CopyIcon /></AddressButton>
            </Tooltip>
            <ButtonRow>
              <NavButton
                onSubmit={onSubmitUpdateName}
                disabled={!accountName}
                text={locale.accountSettingsSave}
                buttonType='secondary'
              />
              {/* <NavButton
                onSubmit={onRemoveAccount}
                text={locale.accountSettingsRemove}
                buttonType='danger'
              /> */}
            </ButtonRow>
          </>
        }
        {tab === 'watchlist' &&
          <>
            {fullAssetList.length === 0 ? (
              <LoadingWrapper>
                <LoadIcon />
              </LoadingWrapper>
            ) : (
              <>
                <SearchBar
                  placeholder={locale.watchListSearchPlaceholder}
                  action={filterWatchlist}
                />
                <WatchlistScrollContainer>
                  {filteredWatchlist.map((item) =>
                    <AssetWatchlistItem
                      key={item.contractAddress}
                      id={item.contractAddress}
                      assetBalance='0'
                      icon={item.icon}
                      name={item.name}
                      symbol={item.symbol}
                      isSelected={isAssetSelected(item.contractAddress)}
                      onSelectAsset={onCheckWatchlistItem}
                    />
                  )}
                </WatchlistScrollContainer>
                <NavButton
                  onSubmit={onClickUpdateVisibleTokens}
                  text={locale.watchlistButton}
                  buttonType='primary'
                />
              </>
            )}
          </>
        }
      </StyledWrapper>
    </PopupModal>
  )
}

export default AddAccountModal
