/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Types
import { SetAdvancedViewFirstAccess } from '../../../types/actions/shieldsPanelActions'

// Feature-specific components
import {
  Overlay,
  WarningModal,
  WarningIcon,
  WarningText,
  ShieldsButton
} from 'brave-ui/features/shields'

// Shared components
import { Card } from 'brave-ui'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

interface Props {
  setAdvancedViewFirstAccess: SetAdvancedViewFirstAccess
}

export default class WebCompatWarning extends React.PureComponent<Props, {}> {
  setAdvancedViewFirstAccess = () => {
    this.props.setAdvancedViewFirstAccess()
  }
  render () {
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
              onClick={this.setAdvancedViewFirstAccess}
              text={getLocale('gotIt')}
            />
          </WarningModal>
        </Card>
      </Overlay>
    )
  }
}
