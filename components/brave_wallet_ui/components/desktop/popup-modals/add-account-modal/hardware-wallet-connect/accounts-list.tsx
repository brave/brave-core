import * as React from 'react'
import { create } from 'ethereum-blockies'
import { Checkbox, Select } from 'brave-ui/components'
import {
  ButtonsContainer,
  DisclaimerWrapper,
  HardwareWalletAccountCircle,
  HardwareWalletAccountListItem,
  HardwareWalletAccountListItemRow,
  HardwareWalletAccountsList,
  SelectRow,
  SelectWrapper,
  LoadingWrapper,
  LoadIcon,
  AddressBalanceWrapper,
  NoSearchResultText
} from './style'
import {
  HardwareWalletDerivationPathLocaleMapping,
  HardwareWalletDerivationPathsMapping
} from './types'
import {
  FilecoinNetwork,
  FilecoinNetworkTypes,
  FilecoinNetworkLocaleMapping
} from '../../../../../common/hardware/types'
import { BraveWallet, WalletAccountType, CreateAccountOptionsType } from '../../../../../constants/types'
import { getLocale } from '../../../../../../common/locale'
import { NavButton } from '../../../../extension'
import { SearchBar } from '../../../../shared'
import { DisclaimerText } from '../style'

// Utils
import { reduceAddress } from '../../../../../utils/reduce-address'
import Amount from '../../../../../utils/amount'

interface Props {
  hardwareWallet: string
  accounts: BraveWallet.HardwareWalletAccount[]
  selectedNetwork: BraveWallet.NetworkInfo
  preAddedHardwareWalletAccounts: WalletAccountType[]
  onLoadMore: () => void
  selectedDerivationPaths: string[]
  setSelectedDerivationPaths: (paths: string[]) => void
  selectedDerivationScheme: string
  setSelectedDerivationScheme: (scheme: string) => void
  onAddAccounts: () => void
  getBalance: (address: string, coin: BraveWallet.CoinType) => Promise<string>
  filecoinNetwork: FilecoinNetwork
  onChangeFilecoinNetwork: (network: FilecoinNetwork) => void
  selectedAccountType: CreateAccountOptionsType
}

export default function (props: Props) {
  const {
    accounts,
    selectedNetwork,
    preAddedHardwareWalletAccounts,
    hardwareWallet,
    selectedDerivationScheme,
    setSelectedDerivationScheme,
    setSelectedDerivationPaths,
    selectedDerivationPaths,
    onLoadMore,
    onAddAccounts,
    getBalance,
    filecoinNetwork,
    onChangeFilecoinNetwork,
    selectedAccountType
  } = props
  const [filteredAccountList, setFilteredAccountList] = React.useState<BraveWallet.HardwareWalletAccount[]>([])
  const [isLoadingMore, setIsLoadingMore] = React.useState<boolean>(false)

  React.useMemo(() => {
    setFilteredAccountList(accounts)
    setIsLoadingMore(false)
  }, [accounts])

  const derivationPathsEnum = HardwareWalletDerivationPathsMapping[hardwareWallet]

  const onSelectAccountCheckbox = (account: BraveWallet.HardwareWalletAccount) => () => {
    const { derivationPath } = account
    const isSelected = selectedDerivationPaths.includes(derivationPath)
    const updatedPaths = isSelected
      ? selectedDerivationPaths.filter((path) => path !== derivationPath)
      : [...selectedDerivationPaths, derivationPath]
    setSelectedDerivationPaths(updatedPaths)
  }

  const filterAccountList = (event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event?.target?.value || ''
    if (search === '') {
      setFilteredAccountList(accounts)
    } else {
      const filteredList = accounts.filter((account) => {
        return (
          account.address.toLowerCase() === search.toLowerCase() ||
          account.address.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setFilteredAccountList(filteredList)
    }
  }

  const onClickLoadMore = () => {
    setIsLoadingMore(true)
    onLoadMore()
  }

  const isPreAddedAccount = React.useCallback((account: BraveWallet.HardwareWalletAccount) => {
    return preAddedHardwareWalletAccounts.some(e => e.address === account.address)
  }, [preAddedHardwareWalletAccounts])

  return (
    <>
      <SelectRow>
        <SelectWrapper>
          {selectedAccountType.coin !== BraveWallet.CoinType.FIL ? (
            <Select value={selectedDerivationScheme} onChange={setSelectedDerivationScheme}>
              {Object.keys(derivationPathsEnum).map((path, index) => {
                const pathValue = derivationPathsEnum[path]
                const pathLocale = HardwareWalletDerivationPathLocaleMapping[pathValue]
                return (
                  <div data-value={pathValue} key={index}>
                    {pathLocale}
                  </div>
                )
              })}
            </Select>
          ) : (
            <Select value={filecoinNetwork} onChange={onChangeFilecoinNetwork}>
              {FilecoinNetworkTypes.map((network, index) => {
                const networkLocale = FilecoinNetworkLocaleMapping[network]
                return (
                  <div data-value={network} key={index}>
                    {networkLocale}
                  </div>
                )
              })}
            </Select>
          )}
        </SelectWrapper>
      </SelectRow>
      <DisclaimerWrapper>
        <DisclaimerText>{getLocale('braveWalletSwitchHDPathTextHardwareWallet')}</DisclaimerText>
      </DisclaimerWrapper>
      <SearchBar placeholder={getLocale('braveWalletSearchScannedAccounts')} action={filterAccountList} />
      <HardwareWalletAccountsList>
        {
          accounts.length === 0 && (
            <LoadingWrapper>
              <LoadIcon size='big' />
            </LoadingWrapper>
          )
        }

        {
          accounts.length > 0 && filteredAccountList?.length === 0 && (
            <NoSearchResultText>
              {getLocale('braveWalletConnectHardwareSearchNothingFound')}
            </NoSearchResultText>
          )
        }

        {
          accounts.length > 0 && filteredAccountList.length > 0 && (
            <>
              {filteredAccountList?.map((account) => {
                return (
                  <AccountListItem
                    key={account.derivationPath}
                    selectedNetwork={selectedNetwork}
                    account={account}
                    selected={
                      selectedDerivationPaths.includes(account.derivationPath) ||
                      isPreAddedAccount(account)
                    }
                    disabled={isPreAddedAccount(account)}
                    onSelect={onSelectAccountCheckbox(account)}
                    getBalance={getBalance}
                  />
                )
              })}
            </>
          )
        }

      </HardwareWalletAccountsList>
      <ButtonsContainer>
        <NavButton
          onSubmit={onClickLoadMore}
          text={isLoadingMore ? getLocale('braveWalletLoadingMoreAccountsHardwareWallet')
            : getLocale('braveWalletLoadMoreAccountsHardwareWallet')}
          buttonType='primary'
          disabled={isLoadingMore || accounts.length === 0}
        />
        <NavButton
          onSubmit={onAddAccounts}
          text={getLocale('braveWalletAddCheckedAccountsHardwareWallet')}
          buttonType='primary'
          disabled={accounts.length === 0}
        />
      </ButtonsContainer>
    </>
  )
}

interface AccountListItemProps {
  account: BraveWallet.HardwareWalletAccount
  selectedNetwork: BraveWallet.NetworkInfo
  onSelect: () => void
  selected: boolean
  disabled: boolean
  getBalance: (address: string, coin: BraveWallet.CoinType) => Promise<string>
}

function AccountListItem (props: AccountListItemProps) {
  const { account, onSelect, selected, disabled, getBalance, selectedNetwork } = props
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])
  const [balance, setBalance] = React.useState('')

  React.useMemo(() => {
    getBalance(account.address, account.coin).then((result) => {
      const formattedBalance = new Amount(result)
        .divideByDecimals(selectedNetwork.decimals)
        .format()
      setBalance(formattedBalance)
    }).catch()
  }, [account])

  return (
    <HardwareWalletAccountListItem>
      <HardwareWalletAccountCircle orb={orb} />
      <HardwareWalletAccountListItemRow>
        <AddressBalanceWrapper>
          <div>{reduceAddress(account.address)}</div>
        </AddressBalanceWrapper>
        <AddressBalanceWrapper>{balance}</AddressBalanceWrapper>
        <Checkbox
          value={{ selected }}
          onChange={onSelect}
          disabled={disabled}
        >
          <div data-key='selected' />
        </Checkbox>
      </HardwareWalletAccountListItemRow>
    </HardwareWalletAccountListItem>
  )
}
