/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Modal, ModalCloseButton } from '../modal'
import { RewardsTour } from './rewards_tour'
import { RewardsTourProps } from './rewards_tour_props'
import * as style from './rewards_tour_modal.style'

interface Props extends RewardsTourProps {
  onClose: () => void
}

export function RewardsTourModal (props: Props) {
  return (
    <Modal>
      <style.root className={`tour-modal-${props.layout || 'narrow'}`}>
        <ModalCloseButton onClick={props.onClose} />
        <style.content>
          <RewardsTour {...props} />
        </style.content>
      </style.root>
    </Modal>
  )
}
