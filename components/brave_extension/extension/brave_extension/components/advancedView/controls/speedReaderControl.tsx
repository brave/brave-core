/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRow,
  BlockedInfoRowData,
  BlockedInfoRowText,
  Toggle
} from 'brave-ui/features/shields'

// Helpers
import {
  getToggleStateViaEventTarget
} from '../../../helpers/shieldsUtils'

// Types
import { 
  SpeedReaderToggled, 
} from '../../../types/actions/shieldsPanelActions'
import { BlockOptions } from '../../../types/other/blockTypes'

interface CommonProps {
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  hostname: string
  favicon: string
}

interface SpeedReaderProps {
  speedreader: BlockOptions
  speedReaderToggled: SpeedReaderToggled
}

export type Props = CommonProps & SpeedReaderProps

interface State {
  speedReaderOpen: boolean
}

export default class SpeedReaderControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    // this comes from the API. if speedreader is note enable in the settings, the toggle does not show up here
    this.state = { speedReaderOpen: false }
  }

  get shouldEnableSpeedReader (): boolean {
    const { speedreader } = this.props
    return speedreader === 'block'
  }

  get maybeDisableResourcesRow (): boolean {
    // return this.state.speedReaderToggled
    return false;
  }

  triggerSpeedReaderEnabled = (
    event: React.MouseEvent<HTMLDivElement> | React.KeyboardEvent<HTMLDivElement>
  ) => {
    event.currentTarget.blur()
    // this.setState({ speedReaderToggled: !this.state.speedReaderToggled })
  }

  // onSpeedReaderEnabled = (event: React.MouseEvent<HTMLDivElement>) => {
  //   this.triggerSpeedReaderEnabled(event)
  // }

  onSpeedReaderEnabledViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event.key === ' ') {
      this.triggerSpeedReaderEnabled(event)
    }
  }

  onChangeSpeedReaderEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    //const shouldEnableSpeedReader = event.target.checked ? 'allow' : 'block'
    console.log("on change reader enabled!!" + event.target.checked);
    const shouldEnableSpeedReader = getToggleStateViaEventTarget(event)
    console.log("on change reader enabled!! - should enable " + shouldEnableSpeedReader);
    this.props.speedReaderToggled(shouldEnableSpeedReader) 
  }

  render () {
    // const { isBlockedListOpen } = this.props
    // const { speedReaderToggled } = this.state
    console.log("render")

    return (
      // speedReaderToggled &&
      <>
        <BlockedInfoRow id='speedReaderControl'>
          <BlockedInfoRowData
            disabled={this.maybeDisableResourcesRow}
            // onClick={this.onSpeedReaderEnabled}
            // onKeyDown={this.onSpeedReaderEnabledViaKeyboard}
          >
            <BlockedInfoRowText>SpeedReader enabled</BlockedInfoRowText>
          </BlockedInfoRowData>
          <Toggle
            id='speedReaderToggled'
            size='small'
            // disabled={isBlockedListOpen}
            checked={this.shouldEnableSpeedReader}
            onChange={this.onChangeSpeedReaderEnabled}
          />
        </BlockedInfoRow>
      </>
    )
  }
}
