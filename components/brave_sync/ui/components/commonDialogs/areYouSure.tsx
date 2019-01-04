/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Components
import { AlertBox } from 'brave-ui'

// Feature-specific components
import { Title } from 'brave-ui/features/sync'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  onClickOk: () => void
  onClickCancel: () => void
}

export default class AreYouSure extends React.PureComponent<Props, {}> {
  render () {
    const { onClickOk, onClickCancel } = this.props
    return (
      <AlertBox
        okString={getLocale('ok')}
        onClickOk={onClickOk}
        cancelString={getLocale('cancel')}
        onClickCancel={onClickCancel}
      >
        <Title level={1}>{getLocale('areYouSure')}</Title>
      </AlertBox>
    )
  }
}
