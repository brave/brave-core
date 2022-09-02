import * as React from 'react'
import {
  useDispatch,
  useSelector
} from 'react-redux'

// Actions
import { PanelActions } from '../../../panel/actions'

// Types
import { BraveWallet, WalletAccountType, PanelState, WalletState } from '../../../constants/types'

// Components
import {
  DividerLine,
  SelectAddress,
  ConnectBottomNav,
  ConnectHeader
} from '../'

// Styled Components
import {
  StyledWrapper,
  SelectAddressContainer,
  SelectAddressInnerContainer,
  NewAccountTitle,
  SelectAddressScrollContainer,
  Details,
  MiddleWrapper,
  AccountListWrapper,
  ConfirmTextRow,
  ConfirmTextColumn,
  ConfirmText,
  ConfirmIcon
} from './style'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getLocale } from '../../../../common/locale'

export interface Props {
  originInfo: BraveWallet.OriginInfo
  accountsToConnect: WalletAccountType[]
}
export const ConnectWithSite = (props: Props) => {
  const {
    originInfo,
    accountsToConnect
  } = props

  // Redux
  const dispatch = useDispatch()

  // Wallet State
  const defaultAccounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultAccounts)
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)

  // Panel State
  const connectingAccounts = useSelector(({ panel }: { panel: PanelState }) => panel.connectingAccounts)

  // State
  const [selectedAccounts, setSelectedAccounts] = React.useState<
    WalletAccountType[] | undefined
  >(undefined)
  const [readyToConnect, setReadyToConnect] = React.useState<boolean>(false)

  // Methods
  const onNext = React.useCallback(() => {
    if (!readyToConnect) {
      setReadyToConnect(true)
      return
    }
    if (selectedAccounts) {
      dispatch(PanelActions.connectToSite({ selectedAccounts }))
      setSelectedAccounts([])
      setReadyToConnect(false)
    }
  }, [readyToConnect, selectedAccounts, dispatch])

  const onBack = React.useCallback(() => {
    if (readyToConnect) {
      setReadyToConnect(false)
      return
    }
    if (selectedAccounts) {
      dispatch(PanelActions.cancelConnectToSite({ selectedAccounts }))
      setSelectedAccounts([])
      setReadyToConnect(false)
    }
  }, [readyToConnect, selectedAccounts, dispatch])

  const checkIsSelected = React.useCallback((account: WalletAccountType) => {
    return selectedAccounts?.some((a) => a.id === account.id) ?? false
  }, [selectedAccounts])

  const toggleSelected = React.useCallback((account: WalletAccountType) => () => {
    if (checkIsSelected(account)) {
      const removedList = selectedAccounts?.filter(
        (accounts) => accounts.id !== account.id
      )
      setSelectedAccounts(removedList)
      return
    }
    if (selectedAccounts) {
      const addedList = [...selectedAccounts, account]
      setSelectedAccounts(addedList)
    }
  }, [selectedAccounts, checkIsSelected])

  // Memos
  const accountsToConnectList: string | undefined = React.useMemo(() => {
    const list = selectedAccounts?.map((a) => {
      return reduceAddress(a.address)
    })
    return list?.join(', ')
  }, [selectedAccounts])

  const defaultAccount: WalletAccountType | undefined = React.useMemo(() => {
    const foundDefaultAccountInfo = defaultAccounts.find((account) =>
      connectingAccounts.includes(account.address.toLowerCase())
    )
    return accounts.find(
      (account) =>
        account.address.toLowerCase() ===
        foundDefaultAccountInfo?.address?.toLowerCase() ?? ''
    )
  }, [defaultAccounts, connectingAccounts, accounts])

  // Update on render
  let ignore = false
  if (
    selectedAccounts === undefined &&
    defaultAccount !== undefined &&
    !ignore
  ) {
    setSelectedAccounts([defaultAccount])
    ignore = true
  }

  // Effects
  const refs = React.useRef<Array<HTMLDivElement | null>>([])
  React.useEffect(() => {
    // Scroll to the first element that was selected
    refs.current.every((ref) => {
      if (ref) {
        ref.scrollIntoView({
          behavior: 'smooth',
          block: 'center',
          inline: 'center'
        })
        return false
      }
      return true
    })
  }, [])

  return (
    <StyledWrapper>
      <ConnectHeader originInfo={originInfo} />
      <MiddleWrapper>
        {readyToConnect ? (
          <AccountListWrapper>
            <Details>{accountsToConnectList}</Details>
          </AccountListWrapper>
        ) : (
          <Details>{getLocale('braveWalletConnectWithSiteTitle')}</Details>
        )}
        {!readyToConnect ? (
          <SelectAddressContainer>
            <NewAccountTitle>{getLocale('braveWalletAccounts')}</NewAccountTitle>
            <DividerLine />
            <SelectAddressScrollContainer>
              {accountsToConnect.map((account, index) => (
                <SelectAddressInnerContainer
                  key={account.id}
                  ref={(ref) => refs.current[index] = (checkIsSelected(account) ? ref : null)}
                >
                  <SelectAddress
                    action={toggleSelected(account)}
                    account={account}
                    isSelected={checkIsSelected(account)}
                  />
                </SelectAddressInnerContainer>
              ))}
            </SelectAddressScrollContainer>
            <DividerLine />
          </SelectAddressContainer>
        ) : (
          <ConfirmTextRow>
            <ConfirmIcon />
            <ConfirmTextColumn>
              <ConfirmText>{getLocale('braveWalletConnectWithSiteDescription')}</ConfirmText>
            </ConfirmTextColumn>
          </ConfirmTextRow>
        )}
      </MiddleWrapper>
      <ConnectBottomNav
        primaryText={readyToConnect ? getLocale('braveWalletAddAccountConnect') : getLocale('braveWalletConnectWithSiteNext')}
        secondaryText={readyToConnect ? getLocale('braveWalletBack') : getLocale('braveWalletButtonCancel')}
        primaryAction={onNext}
        secondaryAction={onBack}
        disabled={selectedAccounts?.length === 0}
      />
    </StyledWrapper>
  )
}

export default ConnectWithSite
