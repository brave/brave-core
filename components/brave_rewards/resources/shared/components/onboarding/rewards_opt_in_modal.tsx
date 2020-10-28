/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { BatColorIcon, WalletAddIcon } from 'brave-ui/components/icons'

import { LocaleContext } from '../../lib/locale_context'
import { Modal } from '../modal'
import { CloseIcon } from '../icons/close_icon'
import { TermsOfService } from './terms_of_service'
import { MainButton } from './main_button'
import { IdIcon } from './icons/id_icon'
import { CameraIcon } from './icons/camera_icon'

import * as style from './rewards_opt_in_modal.style'

interface Props {
  onClose: () => void
  onEnable: () => void
  onAddFunds: () => void
}

export function RewardsOptInModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [showAddFunds, setShowAddFunds] = React.useState(false)

  const onClickAddFunds = () => setShowAddFunds(true)

  const onClickClose = () => {
    if (showAddFunds) {
      setShowAddFunds(false)
    } else {
      props.onClose()
    }
  }

  const renderOptIn = () => (
    <>
      <style.header>
        <style.batIcon><BatColorIcon /></style.batIcon>
        {getString('onboardingEarnHeader')}
      </style.header>
      <style.text>
        {getString('onboardingEarnText')}
      </style.text>
      <style.enable>
        <MainButton onClick={props.onEnable}>
          {getString('onboardingStartUsingRewards')}
        </MainButton>
      </style.enable>
      <style.addFunds>
        <button onClick={onClickAddFunds}>
          <style.addFundsIcon>
            <WalletAddIcon />
          </style.addFundsIcon>
          {getString('onboardingAddFunds')}
        </button>
      </style.addFunds>
    </>
  )

  const renderAddFunds = () => (
    <>
      <style.header>
        {getString('onboardingAddFundsHeader')}
      </style.header>
      <style.text>
        {getString('onboardingAddFundsText')}
      </style.text>
      <style.addFundsItem>
        <IdIcon />{getString('onboardingAddFundsId')}
      </style.addFundsItem>
      <style.addFundsItem>
        <CameraIcon />{getString('onboardingAddFundsPhoto')}
      </style.addFundsItem>
      <style.addFundsAction>
        <MainButton onClick={props.onAddFunds}>
          {getString('onboardingGoToUphold')}
        </MainButton>
      </style.addFundsAction>
    </>
  )

  return (
    <Modal>
      <style.root>
        <style.close>
          <button onClick={onClickClose}><CloseIcon /></button>
        </style.close>
        {showAddFunds ? renderAddFunds() : renderOptIn()}
        <style.terms>
          <TermsOfService />
        </style.terms>
      </style.root>
    </Modal>
  )
}
