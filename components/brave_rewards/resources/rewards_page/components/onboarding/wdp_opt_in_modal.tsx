/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'

import { useLocaleContext } from '../../lib/locale_strings'
import { formatMessage } from '../../../shared/lib/locale_context'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import { Modal } from '../modal'
import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './wdp_opt_in_modal.style'

interface Props {
  onContinue: (enableWdp: boolean) => void
}

export function WdpOptInModal(props: Props) {
  const { getString } = useLocaleContext()
  const [checked, setChecked] = React.useState(true)

  function onContinue() {
    props.onContinue(checked)
  }

  return (
    <Modal className='wdp-opt-in-modal'>
      <div {...style}>
        <div className='graphic' />
        <h3>{getString('wdpOptInTitle')}</h3>
        <p>{getString('wdpOptInText')}</p>
        <Checkbox
          checked={checked}
          onChange={({ checked }) => setChecked(checked)}
        >
          <div className='checkbox-text'>
            {
              formatMessage(getString('wdpCheckboxLabel'), {
                tags: {
                  $1: (content) => (
                    <NewTabLink key='link' href={urls.wdpLearnMoreURL}>
                      {content}
                    </NewTabLink>
                  )
                }
              })
            }
          </div>
        </Checkbox>
        <div className='actions'>
          <Button className='continue-button' onClick={onContinue}>
            {getString('continueButtonLabel')}
          </Button>
        </div>
      </div>
    </Modal>
  )
}
