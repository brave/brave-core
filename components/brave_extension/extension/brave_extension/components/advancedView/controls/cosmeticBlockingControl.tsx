/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRow,
  BlockedInfoRowSingleText,
  //BlockedInfoRowData,
  Toggle
} from 'brave-ui/features/shields'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Helpers
import {
  maybeBlockResource,
  getToggleStateViaEventTarget,
} from '../../../helpers/shieldsUtils'

// Types
import { HideCosmeticElements } from '../../../types/actions/shieldsPanelActions'
import { BlockOptions } from '../../../types/other/blockTypes'

interface CommonProps {
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
}

interface CosmeticBlockingProps {
  cosmeticBlocking: BlockOptions
  hideCosmeticElements: HideCosmeticElements
}

export type Props = CommonProps & CosmeticBlockingProps

interface State {}

export default class CosmeticBlockingControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {}
  }

  get maybeBlock1stPartyTrackersBlocked (): boolean {
    const { cosmeticBlocking } = this.props
    return maybeBlockResource(cosmeticBlocking)
  }

  onChange1stPartyTrackersBlockedEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    const shouldEnableCosmeticBlocking = getToggleStateViaEventTarget(event)
    this.props.hideCosmeticElements(shouldEnableCosmeticBlocking)
  }

  render () {
    const { isBlockedListOpen } = this.props
    return (
      <>
        <BlockedInfoRow id='cosmeticBlockerControl'>
            <BlockedInfoRowSingleText>{getLocale('firstPartyTrackersBlocked')}</BlockedInfoRowSingleText>
            <Toggle
              id='cosmeticFiltering'
              size='small'
              disabled={isBlockedListOpen}
              checked={this.maybeBlock1stPartyTrackersBlocked}
              onChange={this.onChange1stPartyTrackersBlockedEnabled}
            />
        </BlockedInfoRow>
      </>
    )
  }
}
