import * as React from 'react'
import { create } from 'ethereum-blockies'

// types
import { BraveWallet, UserAccountType } from '../../../constants/types'

// utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'

// components
import { Tooltip } from '../tooltip/index'
import { CreateNetworkIcon } from '../create-network-icon/index'

// style
import {
  IconsWrapper,
  NetworkIconWrapper,
  SwitchAccountIcon,
  Row,
  HorizontalSpace
} from '../style'
import {
  StyledWrapper,
  AccountAddress,
  AccountAndAddress,
  AccountCircle,
  AccountName,
  LeftSide,
  BigCheckMark
} from './style'

export interface Props {
  account: UserAccountType
  selectedAccount?: UserAccountType
  selectedNetwork?: BraveWallet.NetworkInfo
  onSelectAccount?: () => void
  showTooltips?: boolean
  fullAddress?: boolean
  hideAddress?: boolean
  showSwitchAccountsIcon?: boolean
}

export function SelectAccountItem ({
  account,
  selectedAccount,
  onSelectAccount,
  showTooltips,
  fullAddress,
  selectedNetwork,
  hideAddress,
  showSwitchAccountsIcon: showSwitchAccountsLink
}: Props) {
  // methods
  const onKeyPress = React.useCallback(({ key }: React.KeyboardEvent) => {
    // Invoke for space or enter, just like a regular input or button
    if (onSelectAccount && [' ', 'Enter'].includes(key)) {
      onSelectAccount()
    }
  }, [onSelectAccount])

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: account.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [account])

  const PossibleToolTip = React.useMemo(() => {
    return showTooltips ? Tooltip : ({ children }: React.PropsWithChildren<{
      text: string
      isAddress?: boolean
    }>) => <>{children}</>
  }, [showTooltips])

  // render
  return (
    <StyledWrapper onKeyPress={onKeyPress}>
      <LeftSide>
        {!selectedNetwork && <AccountCircle orb={orb} onClick={onSelectAccount} />}
        {selectedNetwork &&
          <IconsWrapper onClick={onSelectAccount}>
            <AccountCircle orb={orb} style={{ width: '36px', height: '36px' }} />
            <NetworkIconWrapper>
              <CreateNetworkIcon size='small' network={selectedNetwork} />
            </NetworkIconWrapper>
          </IconsWrapper>
        }
        <AccountAndAddress>

          <PossibleToolTip text={account.name} isAddress>
            <Row justifyContent={'flex-start'}>
              <AccountName>{reduceAccountDisplayName(account.name, 22)}</AccountName>
              {showSwitchAccountsLink &&
                <>
                  <HorizontalSpace space='5px' />
                  <SwitchAccountIcon onClick={onSelectAccount} />
                </>
              }
            </Row>
          </PossibleToolTip>

          {!hideAddress &&
            <PossibleToolTip text={account.address} isAddress>
              <AccountAddress>{fullAddress
                ? account.address
                : reduceAddress(account.address)
              }</AccountAddress>
            </PossibleToolTip>
          }

        </AccountAndAddress>
      </LeftSide>
      {account.address.toLowerCase() === selectedAccount?.address.toLowerCase() &&
        <BigCheckMark />
      }
    </StyledWrapper>
  )
}

export default SelectAccountItem
