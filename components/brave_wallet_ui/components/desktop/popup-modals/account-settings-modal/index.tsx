import * as React from 'react'
import * as qr from 'qr-image'
import {
  AccountSettingsNavTypes,
  WalletAccountType,
  UserAssetOptionType,
  AssetOptionType
} from '../../../../constants/types'
import {
  PopupModal,
  TopTabNav,
  AssetWatchlistItem
} from '../..'
import { AccountSettingsNavOptions } from '../../../../options/account-settings-nav-options'
import { AssetOptions } from '../../../../options/asset-options'
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
  WatchlistScrollContainer
} from './style'

export interface Props {
  onClose: () => void
  onUpdateAccountName: (name: string) => void
  onUpdateWatchList: (list: string[]) => void
  onCopyToClipboard: () => void
  onChangeTab: (id: AccountSettingsNavTypes) => void
  userAssetList: UserAssetOptionType[]
  userWatchList: string[]
  tab: AccountSettingsNavTypes
  title: string
  account: WalletAccountType
}

const AddAccountModal = (props: Props) => {
  const {
    title,
    account,
    tab,
    userWatchList,
    onClose,
    onUpdateAccountName,
    onUpdateWatchList,
    onCopyToClipboard,
    onChangeTab
  } = props
  const [accountName, setAccountName] = React.useState<string>(account.name)
  const [filteredWatchlist, setFilteredWatchlist] = React.useState<AssetOptionType[]>(AssetOptions)
  const [selectedWatchlist, setSelectedWatchlist] = React.useState<string[]>(userWatchList)
  const [qrCode, setQRCode] = React.useState<string>('')

  const filterWatchlist = (event: any) => {
    const search = event.target.value
    if (search === '') {
      setFilteredWatchlist(AssetOptions)
    } else {
      const filteredList = AssetOptions.filter((item) => {
        return (
          item.name.toLowerCase() === search.toLowerCase() ||
          item.name.toLowerCase().startsWith(search.toLowerCase()) ||
          item.symbol.toLocaleLowerCase() === search.toLowerCase() ||
          item.symbol.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setFilteredWatchlist(filteredList)
    }
  }

  const onClickUpdateWatchlist = () => {
    onUpdateWatchList(selectedWatchlist)
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
    onUpdateAccountName(accountName)
  }

  const generateQRData = () => {
    const image = qr.image(account.address)
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

  return (
    <PopupModal title={title} onClose={onClose}>
      <TopTabNav
        tabList={AccountSettingsNavOptions}
        onSubmit={onChangeTab}
        selectedTab={tab}
      />

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
              <AddressButton onClick={onCopyToClipboard}>{reduceAddress(account.address)}<CopyIcon /></AddressButton>
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
            <SearchBar
              placeholder={locale.watchListSearchPlaceholder}
              action={filterWatchlist}
            />
            <WatchlistScrollContainer>
              {filteredWatchlist.map((item) =>
                <AssetWatchlistItem
                  key={item.id}
                  id={item.id}
                  assetBalance='0'
                  icon={item.icon}
                  name={item.name}
                  symbol={item.symbol}
                  isSelected={isAssetSelected(item.id)}
                  onSelectAsset={onCheckWatchlistItem}
                />
              )}
            </WatchlistScrollContainer>
            <NavButton
              onSubmit={onClickUpdateWatchlist}
              text={locale.watchlistButton}
              buttonType='primary'
            />
          </>
        }
      </StyledWrapper>
    </PopupModal>
  )
}

export default AddAccountModal
