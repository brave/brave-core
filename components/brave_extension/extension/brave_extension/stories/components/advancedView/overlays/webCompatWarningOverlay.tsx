/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Overlay,
  WarningModal,
  WarningIcon,
  WarningText,
  ShieldsButton
} from '../../../../components'

// Shared components
import { Card } from 'brave-ui'

// Helpers
import { getLocale } from '../../../fakeLocale'

interface Props {
  onConfirm: () => void
}

export default class WebCompatWarning extends React.PureComponent<Props, {}> {
  render () {
    const { onConfirm } = this.props
    return (
      <Overlay>
        <Card>
          <WarningModal>
            <WarningIcon />
            <WarningText>
              {getLocale('webCompatWarning')}
            </WarningText>
            <ShieldsButton
              level='primary'
              type='accent'
              onClick={onConfirm}
              text={getLocale('gotIt')}
            />
          </WarningModal>
        </Card>
      </Overlay>
    )
  }
}
