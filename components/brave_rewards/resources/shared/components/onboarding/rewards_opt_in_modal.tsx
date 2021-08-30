/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { Modal, ModalCloseButton } from '../modal'
import { TermsOfService } from '../terms_of_service'
import { BatIcon } from '../icons/bat_icon'
import { MainButton } from './main_button'

import * as style from './rewards_opt_in_modal.style'

interface Props {
  onClose?: () => void
  onTakeTour: () => void
  onEnable: () => void
}

export function RewardsOptInModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  return (
    <Modal>
      <style.root>
        {props.onClose && <ModalCloseButton onClick={props.onClose} />}
        <style.header>
          <BatIcon />{getString('onboardingEarnHeader')}
        </style.header>
        <style.text>
          {getString('onboardingEarnText')}
        </style.text>
        <style.takeTour>
          <button onClick={props.onTakeTour}>
            {getString('onboardingTakeTour')}
          </button>
        </style.takeTour>
        <style.enable>
          <MainButton onClick={props.onEnable}>
            {getString('onboardingStartUsingRewards')}
          </MainButton>
        </style.enable>
        <style.terms>
          <TermsOfService />
        </style.terms>
      </style.root>
    </Modal>
  )
}
