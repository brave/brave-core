import * as React from 'react'
import { create } from 'ethereum-blockies'
import { Checkbox, Select } from 'brave-ui/components'
import {
  ButtonsContainer,
  DisclaimerWrapper,
  HardwareWalletAccountCircle,
  HardwareWalletAccountListItem,
  HardwareWalletAccountListItemColumn,
  HardwareWalletAccountsList,
  SelectWrapper,
  LoadingWrapper,
  LoadIcon,
  AddressBalanceWrapper
} from './style'
import {
  HardwareWalletAccount,
  HardwareWalletDerivationPathLocaleMapping,
  HardwareWalletDerivationPathsMapping
} from './types'
import { reduceAddress } from '../../../../../utils/reduce-address'
import { getLocale } from '../../../../../../common/locale'
import { NavButton } from '../../../../extension'
import { SearchBar } from '../../../../shared'
import { DisclaimerText } from '../style'

interface Props {
  hardwareWallet: string
  accounts: Array<HardwareWalletAccount>
  onLoadMore: () => void
  selectedDerivationPaths: string[]
  setSelectedDerivationPaths: (paths: string[]) => void
  selectedDerivationScheme: string
  setSelectedDerivationScheme: (scheme: string) => void
  onAddAccounts: () => void
  getBalance: (address: string) => Promise<string>
}

export default function (props: Props) {
  const {
    accounts,
    hardwareWallet,
    selectedDerivationScheme,
    setSelectedDerivationScheme,
    setSelectedDerivationPaths,
    selectedDerivationPaths,
    onLoadMore,
    onAddAccounts,
    getBalance
  } = props
  const [filteredAccountList, setFilteredAccountList] = React.useState<HardwareWalletAccount[] | undefined>()
  const [isLoadingMore, setIsLoadingMore] = React.useState<boolean>(false)

  React.useMemo(() => {
    setFilteredAccountList(accounts)
    setIsLoadingMore(false)
  }, [accounts])

  const derivationPathsEnum = HardwareWalletDerivationPathsMapping[hardwareWallet]

  const onSelectAccountCheckbox = (account: HardwareWalletAccount) => () => {
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

  return (
    <>
      <SelectWrapper>
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
      </SelectWrapper>
      <DisclaimerWrapper>
        <DisclaimerText>{getLocale('braveWalletSwitchHDPathTextHardwareWallet')}</DisclaimerText>
      </DisclaimerWrapper>
      <SearchBar placeholder={getLocale('braveWalletSearchScannedAccounts')} action={filterAccountList} />
      <HardwareWalletAccountsList>
        {filteredAccountList?.length === 0 ? (
          <LoadingWrapper>
            <LoadIcon />
          </LoadingWrapper>
        ) : (
          <>
            {filteredAccountList?.map((account) => {
              return (
                <AccountListItem
                  key={account.derivationPath}
                  account={account}
                  selected={selectedDerivationPaths.includes(account.derivationPath)}
                  onSelect={onSelectAccountCheckbox(account)}
                  getBalance={getBalance}
                />
              )
            })}
          </>
        )}
      </HardwareWalletAccountsList>
      <ButtonsContainer>
        <NavButton
          onSubmit={onClickLoadMore}
          text={isLoadingMore ? getLocale('braveWalletLoadingMoreAccountsHardwareWallet')
            : getLocale('braveWalletLoadMoreAccountsHardwareWallet')}
          buttonType='primary'
          disabled={isLoadingMore}
        />
        <NavButton onSubmit={onAddAccounts} text={getLocale('braveWalletAddCheckedAccountsHardwareWallet')} buttonType='primary' />
      </ButtonsContainer>
    </>
  )
}

interface AccountListItemProps {
  account: HardwareWalletAccount
  onSelect: () => void
  selected: boolean
  getBalance: (address: string) => Promise<string>
}

function AccountListItem (props: AccountListItemProps) {
  const { account, onSelect, selected, getBalance } = props
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])
  const [balance, setBalance] = React.useState('')

  React.useMemo(() => {
    getBalance(account.address).then((result) => {
      setBalance(result)
    }).catch()
  }, [account])

  return (
    <HardwareWalletAccountListItem>
      <HardwareWalletAccountCircle orb={orb} />
      <HardwareWalletAccountListItemColumn>
        <AddressBalanceWrapper>
          <div>{reduceAddress(account.address)}</div>
        </AddressBalanceWrapper>
        <AddressBalanceWrapper>{balance}</AddressBalanceWrapper>
        <Checkbox value={{ selected }} onChange={onSelect}>
          <div data-key='selected' />
        </Checkbox>
      </HardwareWalletAccountListItemColumn>
    </HardwareWalletAccountListItem>
  )
}
