/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { BlockedListItem } from '../../../../../../src/features/shields'

// Helpers
import { stripProtocolFromUrl } from '../../../helpers'

interface Props {
  list: any[]
}

export default class StaticResourcesList extends React.PureComponent<Props, {}> {
  render () {
    const { list } = this.props
    return list.map((item, index) =>
      <BlockedListItem key={index}>{stripProtocolFromUrl(item)}</BlockedListItem>
    )
  }
}
