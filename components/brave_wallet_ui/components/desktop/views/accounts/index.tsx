import * as React from 'react'

import {
  WalletAccountType,
  UserAssetOptionType,
  RPCTransactionType,
  AccountSettingsNavTypes
} from '../../../../constants/types'
import { reduceAddress } from '../../../../utils/reduce-address'
import { formatePrices } from '../../../../utils/format-prices'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'
import { create } from 'ethereum-blockies'
import locale from '../../../../constants/locale'

// Styled Components
import {
  StyledWrapper,
  SectionTitle,
  PrimaryListContainer,
  PrimaryRow,
  SecondaryListContainer,
  DisclaimerText,
  SubDivider,
  BackupIcon,
  ButtonsRow,
  SettingsIcon,
  BackupButton,
  BackupButtonText,
  Button,
  TopRow,
  WalletInfoRow,
  WalletAddress,
  WalletName,
  AccountCircle,
  WalletInfoLeftSide,
  QRCodeIcon,
  EditIcon,
  EditButtonRow,
  SubviewSectionTitle
} from './style'

// Components
import { BackButton, Tooltip } from '../../../shared'
import {
  AccountListItem,
  AddButton,
  PortfolioAssetItem,
  AccountSettingsModal,
  PortfolioTransactionItem
} from '../../'

export interface Props {
  userWatchList: string[]
  accounts: WalletAccountType[]
  userAssetList: UserAssetOptionType[]
  transactions: (RPCTransactionType | undefined)[]
  toggleNav: () => void
  onClickBackup: () => void
  onClickAddAccount: () => void
  onUpdateWatchList: (list: string[]) => void
  onUpdateAccountName: (name: string) => void
}

function Accounts (props: Props) {
  const {
    accounts,
    userAssetList,
    transactions,
    userWatchList,
    toggleNav,
    onClickBackup,
    onClickAddAccount,
    onUpdateWatchList,
    onUpdateAccountName
  } = props

  const primaryAccounts = React.useMemo(() => {
    return accounts.filter((account) => account.accountType === 'Primary')
  }, [accounts])

  const secondaryAccounts = React.useMemo(() => {
    return accounts.filter((account) => account.accountType === 'Secondary')
  }, [accounts])

  const hardwareAccounts = React.useMemo(() => {
    return accounts.filter((account) => account.accountType === 'Trezor' || account.accountType === 'Ledger')
  }, [accounts])

  const [selectedAccount, setSelectedAccount] = React.useState<WalletAccountType>()
  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)
  const [editTab, setEditTab] = React.useState<AccountSettingsNavTypes>('details')

  const goBack = () => {
    setSelectedAccount(undefined)
    toggleNav()
  }

  const onSelectAccount = (account: WalletAccountType) => {
    setSelectedAccount(account)
    toggleNav()
  }

  const onCopyToClipboard = async () => {
    if (selectedAccount) {
      await copyToClipboard(selectedAccount.address)
    }
  }

  const onClickSettings = () => {
    alert('Will Nav to Brave Browser Settings')
  }

  const onChangeTab = (id: AccountSettingsNavTypes) => {
    setEditTab(id)
  }

  const toggleShowEditWatchlist = () => {
    setShowEditModal(!showEditModal)
    setEditTab('watchlist')
  }

  const onShowEditModal = () => {
    setShowEditModal(!showEditModal)
  }

  const onCloseEditModal = () => {
    setShowEditModal(!showEditModal)
    setEditTab('details')
  }

  const onTransactionMore = () => {
    alert('Will show view Transaction options')
  }

  const orb = React.useMemo(() => {
    if (selectedAccount) {
      return create({ seed: selectedAccount.address, size: 8, scale: 16 }).toDataURL()
    }
  }, [selectedAccount])

  return (
    <StyledWrapper>
      {selectedAccount &&
        <TopRow><BackButton onSubmit={goBack} /></TopRow>
      }
      {!selectedAccount ? (
        <>
          <PrimaryRow>
            <SectionTitle>{locale.accountsPrimary}</SectionTitle>
            <ButtonsRow>
              <BackupButton onClick={onClickBackup}>
                <BackupIcon />
                <BackupButtonText>{locale.backupButton}</BackupButtonText>
              </BackupButton>
              <Button onClick={onClickSettings}>
                <SettingsIcon />
              </Button>
            </ButtonsRow>
          </PrimaryRow>
          <PrimaryListContainer>
            {primaryAccounts.map((account) =>
              <AccountListItem
                key={account.id}
                isHardwareWallet={false}
                onClick={onSelectAccount}
                account={account}
              />
            )}
          </PrimaryListContainer>
          <SectionTitle>{locale.accountsSecondary}</SectionTitle>
          <DisclaimerText>{locale.accountsSecondaryDisclaimer}</DisclaimerText>
          <SubDivider />
          <SecondaryListContainer isHardwareWallet={false}>
            {secondaryAccounts.map((account) =>
              <AccountListItem
                key={account.id}
                isHardwareWallet={false}
                onClick={onSelectAccount}
                account={account}
              />
            )}
          </SecondaryListContainer>
          {hardwareAccounts.length !== 0 &&
            <SecondaryListContainer isHardwareWallet={true}>
              {hardwareAccounts.map((account) =>
                <AccountListItem
                  key={account.id}
                  isHardwareWallet={true}
                  onClick={onSelectAccount}
                  account={account}
                />
              )}
            </SecondaryListContainer>
          }
          <AddButton
            buttonType='secondary'
            onSubmit={onClickAddAccount}
            text={locale.addAccount}
          />
        </>
      ) : (
        <>
          <WalletInfoRow>
            <WalletInfoLeftSide>
              <AccountCircle orb={orb} />
              <WalletName>{selectedAccount.name}</WalletName>
              <Tooltip text={locale.toolTipCopyToClipboard}>
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
          <SubviewSectionTitle>{locale.accountsWatchlist}</SubviewSectionTitle>
          <SubDivider />
          {userAssetList.map((item) =>
            <PortfolioAssetItem
              key={item.asset.id}
              name={item.asset.name}
              assetBalance={item.assetBalance}
              fiatBalance={formatePrices(item.fiatBalance)}
              symbol={item.asset.symbol}
              icon={item.asset.icon}
            />
          )}
          <EditButtonRow>
            <AddButton
              buttonType='secondary'
              onSubmit={toggleShowEditWatchlist}
              text={locale.accountsEditWatchList}
              editIcon={true}
            />
          </EditButtonRow>
          <SubviewSectionTitle>{locale.transactions}</SubviewSectionTitle>
          <SubDivider />
          {transactions?.map((transaction) =>
            <PortfolioTransactionItem
              action={onTransactionMore}
              key={transaction?.hash}
              amount={transaction?.amount ? transaction.amount : 0}
              from={transaction?.from ? transaction.from : ''}
              to={transaction?.to ? transaction.to : ''}
              ticker={selectedAccount.asset}
            />
          )}
        </>
      )}
      {showEditModal && selectedAccount &&
        <AccountSettingsModal
          userAssetList={userAssetList}
          title={locale.account}
          account={selectedAccount}
          onClose={onCloseEditModal}
          onUpdateAccountName={onUpdateAccountName}
          onUpdateWatchList={onUpdateWatchList}
          onCopyToClipboard={onCopyToClipboard}
          onChangeTab={onChangeTab}
          tab={editTab}
          userWatchList={userWatchList}
        />
      }
    </StyledWrapper>
  )
}

export default Accounts
