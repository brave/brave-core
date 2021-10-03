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
  SelectWrapper
} from './style'
import { HardwareWalletAccount } from '../../../../../constants/types'
import {
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
  const [filteredAccountList, setFilteredAccountList] = React.useState<HardwareWalletAccount[]>(accounts)

  const derivationPathsEnum = HardwareWalletDerivationPathsMapping[hardwareWallet]

  const onSelectAccountCheckbox = (account: HardwareWalletAccount) => () => {
    const { derivationPath } = account
    const isSelected = selectedDerivationPaths.includes(derivationPath)
    const updatedPaths = isSelected
      ? selectedDerivationPaths.filter((path) => path !== derivationPath)
      : [...selectedDerivationPaths, derivationPath]

    setSelectedDerivationPaths(updatedPaths)
  }

  const filterAccountList = (event: any) => {
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

  React.useEffect(() => {
    filterAccountList(null)
  }, [accounts])

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
        {filteredAccountList.map((account) => {
          const { selectedDerivationPaths } = props
          const { derivationPath } = account
          const [balance, setBalance] = React.useState('')
          const isSelected = selectedDerivationPaths.includes(derivationPath)
          getBalance(account.address).then((result) => {
            setBalance(result)
          }).catch()
          return (
            <AccountListItem
              key={derivationPath}
              account={account}
              selected={isSelected}
              balance={balance}
              onSelect={onSelectAccountCheckbox(account)}
            />
          )
        })}
      </HardwareWalletAccountsList>
      <ButtonsContainer>
        <NavButton onSubmit={onLoadMore} text={getLocale('braveWalletLoadMoreAccountsHardwareWallet')} buttonType='primary' />
        <NavButton onSubmit={onAddAccounts} text={getLocale('braveWalletAddCheckedAccountsHardwareWallet')} buttonType='primary' />
      </ButtonsContainer>
    </>
  )
}

interface AccountListItemProps {
  account: HardwareWalletAccount
  onSelect: () => void
  selected: boolean
  balance: string
}

function AccountListItem (props: AccountListItemProps) {
  const { account, onSelect, selected, balance } = props
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account.address])

  return (
    <HardwareWalletAccountListItem>
      <HardwareWalletAccountCircle orb={orb} />
      <HardwareWalletAccountListItemColumn>
        <div>{reduceAddress(account.address)}</div>
        <div>{balance}</div>
        <Checkbox value={{ selected }} onChange={onSelect}>
          <div data-key='selected' />
        </Checkbox>
      </HardwareWalletAccountListItemColumn>
    </HardwareWalletAccountListItem>
  )
}
