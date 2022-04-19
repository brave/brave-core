import * as React from 'react'

import {
  WalletAccountType,
  AccountSettingsNavTypes,
  UpdateAccountNamePayloadType,
  AccountTransactions,
  BraveWallet,
  DefaultCurrencies,
  AddAccountNavTypes,
  CoinTypesMap
} from '../../../../constants/types'
import { reduceAddress } from '../../../../utils/reduce-address'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'
import { create } from 'ethereum-blockies'
import { getLocale } from '../../../../../common/locale'
import { sortTransactionByDate } from '../../../../utils/tx-utils'

// Styled Components
import {
  StyledWrapper,
  SectionTitle,
  PrimaryListContainer,
  SecondaryListContainer,
  DisclaimerText,
  SubDivider,
  Button,
  TopRow,
  WalletInfoRow,
  WalletAddress,
  WalletName,
  AccountCircle,
  WalletInfoLeftSide,
  QRCodeIcon,
  EditIcon,
  SubviewSectionTitle,
  TransactionPlaceholderContainer,
  ButtonRow,
  StyledButton,
  HardwareIcon,
  ButtonText,
  WalletIcon
} from './style'

import { TransactionPlaceholderText, Spacer } from '../portfolio/style'

// Components
import { BackButton, Tooltip } from '../../../shared'
import {
  AccountListItem,
  AddButton,
  PortfolioAssetItem,
  AccountSettingsModal,
  PortfolioTransactionItem
} from '../../'

// Hooks
import { useBalance } from '../../../../common/hooks'

export interface Props {
  accounts: WalletAccountType[]
  transactions: AccountTransactions
  privateKey: string
  networkList: BraveWallet.NetworkInfo[]
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  selectedAccount: WalletAccountType | undefined
  defaultCurrencies: DefaultCurrencies
  onViewPrivateKey: (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => void
  onDoneViewingPrivateKey: () => void
  toggleNav: () => void
  onClickAddAccount: (tabId: AddAccountNavTypes) => () => void
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
  onSelectAccount: (account: WalletAccountType) => void
  goBack: () => void
}

function Accounts (props: Props) {
  const {
    accounts,
    transactions,
    privateKey,
    transactionSpotPrices,
    userVisibleTokensInfo,
    selectedAccount,
    defaultCurrencies,
    networkList,
    goBack,
    onSelectAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    toggleNav,
    onClickAddAccount,
    onUpdateAccountName,
    onRemoveAccount
  } = props

  const getBalance = useBalance(networkList)

  const groupById = (accounts: WalletAccountType[], key: string) => {
    return accounts.reduce((result, obj) => {
      (result[obj[key]] = result[obj[key]] || []).push(obj)
      return result
    }, {})
  }

  const primaryAccounts = React.useMemo(() => {
    return accounts.filter((account) => account.accountType === 'Primary')
  }, [accounts])

  const secondaryAccounts = React.useMemo(() => {
    return accounts.filter((account) => account.accountType === 'Secondary')
  }, [accounts])

  const trezorAccounts = React.useMemo(() => {
    const foundTrezorAccounts = accounts.filter((account) => account.accountType === 'Trezor')
    return groupById(foundTrezorAccounts, 'deviceId')
  }, [accounts])

  const ledgerAccounts = React.useMemo(() => {
    const foundLedgerAccounts = accounts.filter((account) => account.accountType === 'Ledger')
    return groupById(foundLedgerAccounts, 'deviceId')
  }, [accounts])

  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)
  const [editTab, setEditTab] = React.useState<AccountSettingsNavTypes>('details')

  const sortAccountsByName = React.useCallback((accounts: WalletAccountType[]) => {
    return [...accounts].sort(function (a: WalletAccountType, b: WalletAccountType) {
      if (a.name < b.name) {
        return -1
      }

      if (a.name > b.name) {
        return 1
      }

      return 0
    })
  }, [])

  const onCopyToClipboard = async () => {
    if (selectedAccount) {
      await copyToClipboard(selectedAccount.address)
    }
  }

  const onChangeTab = (id: AccountSettingsNavTypes) => {
    setEditTab(id)
  }

  const onShowEditModal = () => {
    setShowEditModal(!showEditModal)
  }

  const onCloseEditModal = () => {
    setShowEditModal(!showEditModal)
    setEditTab('details')
  }

  const orb = React.useMemo(() => {
    if (selectedAccount) {
      return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
    }
  }, [selectedAccount])

  const transactionList = React.useMemo(() => {
    if (selectedAccount?.address && transactions[selectedAccount.address]) {
      return sortTransactionByDate(transactions[selectedAccount.address], 'descending')
    } else {
      return []
    }
  }, [selectedAccount, transactions])

  const accountsTokensList = React.useMemo(() => {
    if (!selectedAccount) {
      return []
    }
    // Since LOCALHOST's chainId is shared between coinType's
    // this check will make sure we are returning the correct
    // LOCALHOST asset for each account.
    const coinName = CoinTypesMap[selectedAccount?.coin ?? 0]
    const localHostCoins = userVisibleTokensInfo.filter((token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID)
    const accountsLocalHost = localHostCoins.find((token) => token.symbol.toUpperCase() === coinName)
    const chainList = networkList.filter((network) => network.coin === selectedAccount?.coin).map((network) => network.chainId) ?? []
    const list =
      userVisibleTokensInfo.filter((token) => chainList.includes(token?.chainId ?? '') &&
        token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID) ?? []
    if (accountsLocalHost) {
      return [...list, accountsLocalHost]
    }
    return list
  }, [userVisibleTokensInfo, selectedAccount, networkList])

  const erc271Tokens = React.useMemo(() =>
    accountsTokensList.filter((token) => token.isErc721),
    [accountsTokensList]
  )

  return (
    <StyledWrapper>
      {selectedAccount &&
        <TopRow><BackButton onSubmit={goBack} /></TopRow>
      }
      {!selectedAccount ? (
        <>
          <SectionTitle>{getLocale('braveWalletAccountsPrimary')}</SectionTitle>
          <DisclaimerText>{getLocale('braveWalletAccountsPrimaryDisclaimer')}</DisclaimerText>
          <SubDivider />
          <PrimaryListContainer>
            {primaryAccounts.map((account) =>
              <AccountListItem
                key={account.id}
                isHardwareWallet={false}
                onClick={onSelectAccount}
                onRemoveAccount={onRemoveAccount}
                account={account}
              />
            )}
          </PrimaryListContainer>
          <ButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={onClickAddAccount('create')}
              text={getLocale('braveWalletCreateAccountButton')}
            />
          </ButtonRow>
          <SectionTitle>{getLocale('braveWalletAccountsSecondary')}</SectionTitle>
          <DisclaimerText>{getLocale('braveWalletAccountsSecondaryDisclaimer')}</DisclaimerText>
          <SubDivider />
          <SecondaryListContainer isHardwareWallet={false}>
            {secondaryAccounts.map((account) =>
              <AccountListItem
                key={account.id}
                isHardwareWallet={false}
                onClick={onSelectAccount}
                onRemoveAccount={onRemoveAccount}
                account={account}
              />
            )}
          </SecondaryListContainer>
          {Object.keys(trezorAccounts).map(key =>
            <SecondaryListContainer key={key} isHardwareWallet={true}>
              {sortAccountsByName(trezorAccounts[key]).map((account: WalletAccountType) =>
                <AccountListItem
                  key={account.id}
                  isHardwareWallet={true}
                  onClick={onSelectAccount}
                  onRemoveAccount={onRemoveAccount}
                  account={account}
                />
              )}
            </SecondaryListContainer>
          )}
          {Object.keys(ledgerAccounts).map(key =>
            <SecondaryListContainer key={key} isHardwareWallet={true}>
              {sortAccountsByName(ledgerAccounts[key]).map((account: WalletAccountType) =>
                <AccountListItem
                  key={account.id}
                  isHardwareWallet={true}
                  onClick={onSelectAccount}
                  onRemoveAccount={onRemoveAccount}
                  account={account}
                />
              )}
            </SecondaryListContainer>
          )}
          <ButtonRow>
            <StyledButton onClick={onClickAddAccount('import')}>
              <WalletIcon />
              <ButtonText>{getLocale('braveWalletAddAccountImport')}</ButtonText>
            </StyledButton>
            <StyledButton onClick={onClickAddAccount('hardware')}>
              <HardwareIcon />
              <ButtonText>{getLocale('braveWalletAddAccountImportHardware')}</ButtonText>
            </StyledButton>
          </ButtonRow>
        </>
      ) : (
        <>
          <WalletInfoRow>
            <WalletInfoLeftSide>
              <AccountCircle orb={orb} />
              <WalletName>{selectedAccount.name}</WalletName>
              <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
                <WalletAddress onClick={onCopyToClipboard}>{reduceAddress(selectedAccount.address)}</WalletAddress>
              </Tooltip>
              <Button onClick={onShowEditModal}>
                <QRCodeIcon />
              </Button>
            </WalletInfoLeftSide>
            <Button onClick={onShowEditModal}>
              <EditIcon />
            </Button>
          </WalletInfoRow>
          <SubviewSectionTitle>{getLocale('braveWalletAccountsAssets')}</SubviewSectionTitle>
          <SubDivider />
          {accountsTokensList.filter((token) => !token.isErc721).map((item) =>
            <PortfolioAssetItem
              spotPrices={transactionSpotPrices}
              defaultCurrencies={defaultCurrencies}
              key={`${item.contractAddress}-${item.symbol}-${item.chainId}`}
              assetBalance={getBalance(selectedAccount, item)}
              networks={networkList}
              token={item}
            />
          )}
          <Spacer />
          {erc271Tokens?.length !== 0 &&
            <>
              <Spacer />
              <SubviewSectionTitle>{getLocale('braveWalletTopNavNFTS')}</SubviewSectionTitle>
              <SubDivider />
              {erc271Tokens?.map((item) =>
                <PortfolioAssetItem
                  spotPrices={transactionSpotPrices}
                  networks={networkList}
                  defaultCurrencies={defaultCurrencies}
                  key={`${item.contractAddress}-${item.symbol}-${item.chainId}`}
                  assetBalance={getBalance(selectedAccount, item)}
                  token={item}
                />
              )}
              <Spacer />
            </>
          }
          <Spacer />
          <SubviewSectionTitle>{getLocale('braveWalletTransactions')}</SubviewSectionTitle>
          <SubDivider />
          {transactionList.length !== 0 ? (
            <>
              {transactionList.map((transaction) =>
                <PortfolioTransactionItem
                  key={transaction?.id}
                  transaction={transaction}
                  account={selectedAccount}
                  accounts={accounts}
                  displayAccountName={false}
                />
              )}
            </>
          ) : (
            <TransactionPlaceholderContainer>
              <TransactionPlaceholderText>{getLocale('braveWalletTransactionPlaceholder')}</TransactionPlaceholderText>
            </TransactionPlaceholderContainer>
          )}
        </>
      )}
      {showEditModal && selectedAccount &&
        <AccountSettingsModal
          title={getLocale('braveWalletAccount')}
          account={selectedAccount}
          onClose={onCloseEditModal}
          onUpdateAccountName={onUpdateAccountName}
          onCopyToClipboard={onCopyToClipboard}
          onChangeTab={onChangeTab}
          onToggleNav={toggleNav}
          onRemoveAccount={onRemoveAccount}
          onViewPrivateKey={onViewPrivateKey}
          onDoneViewingPrivateKey={onDoneViewingPrivateKey}
          privateKey={privateKey}
          tab={editTab}
          hideNav={false}
        />
      }
    </StyledWrapper>
  )
}

export default Accounts
