import * as React from 'react'

import {
  WalletAccountType,
  RPCTransactionType,
  AccountSettingsNavTypes,
  UpdateAccountNamePayloadType
} from '../../../../constants/types'
import { reduceAddress } from '../../../../utils/reduce-address'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'
import { create } from 'ethereum-blockies'
import locale from '../../../../constants/locale'
import { formatBalance } from '../../../../utils/format-balances'

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
  accounts: WalletAccountType[]
  transactions: (RPCTransactionType | undefined)[]
  privateKey: string
  onViewPrivateKey: (address: string, isDefault: boolean) => void
  onDoneViewingPrivateKey: () => void
  toggleNav: () => void
  onClickBackup: () => void
  onClickAddAccount: () => void
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onRemoveAccount: (address: string) => void
}

function Accounts (props: Props) {
  const {
    accounts,
    transactions,
    privateKey,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    toggleNav,
    onClickBackup,
    onClickAddAccount,
    onUpdateAccountName,
    onRemoveAccount
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

  React.useMemo(() => {
    if (selectedAccount) {
      const updatedAccount = accounts.find((account) => account.id === selectedAccount.id)
      setSelectedAccount(updatedAccount)
    }
  }, [accounts])

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
                onRemoveAccount={onRemoveAccount}
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
                onRemoveAccount={onRemoveAccount}
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
                  onRemoveAccount={onRemoveAccount}
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
          <SubviewSectionTitle>{locale.accountsAssets}</SubviewSectionTitle>
          <SubDivider />
          {selectedAccount.tokens.map((item) =>
            <PortfolioAssetItem
              key={item.asset.contractAddress}
              name={item.asset.name}
              assetBalance={formatBalance(item.assetBalance, item.asset.decimals)}
              fiatBalance={item.fiatBalance}
              symbol={item.asset.symbol}
              icon={item.asset.icon}
            />
          )}
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
          title={locale.account}
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
