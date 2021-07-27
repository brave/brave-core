import * as React from 'react'

import locale from '../../../../../constants/locale'
import { NavButton } from '../../../../extension'

// Styled Components
import { DisclaimerText, InfoIcon } from '../style'
import {
  HardwareButton,
  HardwareButtonRow,
  HardwareInfoColumn,
  HardwareInfoRow,
  HardwareTitle,
  LedgerIcon,
  TrezorIcon
} from './style'

enum HardwareWallet {
  Ledger = 'Ledger',
  Trezor = 'Trezor'
}

export default function () {
  const [selectedHardwareWallet, setSelectedHardwareWallet] =
    React.useState<HardwareWallet>(HardwareWallet.Ledger)

  const onConnectHardwareWallet = (hardware: HardwareWallet) => {
    alert(`Connecting to ${hardware}`)
  }

  const onSelectLedger = () => {
    setSelectedHardwareWallet(HardwareWallet.Ledger)
  }

  const onSelectTrezor = () => {
    setSelectedHardwareWallet(HardwareWallet.Trezor)
  }

  const onSubmit = () => onConnectHardwareWallet(selectedHardwareWallet)

  return (
    <>
      <HardwareTitle>{locale.connectHardwareTitle}</HardwareTitle>
      <HardwareButtonRow>
        <HardwareButton
          onClick={onSelectLedger}
          isSelected={selectedHardwareWallet === HardwareWallet.Ledger}
        >
          <LedgerIcon />
        </HardwareButton>
        <HardwareButton
          onClick={onSelectTrezor}
          isSelected={selectedHardwareWallet === HardwareWallet.Trezor}
        >
          <TrezorIcon />
        </HardwareButton>
      </HardwareButtonRow>
      <HardwareInfoRow>
        <InfoIcon />
        <HardwareInfoColumn>
          <DisclaimerText>
            {locale.connectHardwareInfo1} {selectedHardwareWallet}{' '}
            {locale.connectHardwareInfo2}
          </DisclaimerText>
          <DisclaimerText>{locale.connectHardwareInfo3}</DisclaimerText>
        </HardwareInfoColumn>
      </HardwareInfoRow>

      <NavButton
        onSubmit={onSubmit}
        text={locale.addAccountConnect}
        buttonType='primary'
      />
    </>
  )
}
