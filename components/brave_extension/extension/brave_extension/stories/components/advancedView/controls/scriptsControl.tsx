/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRow,
  BlockedInfoRowData,
  ArrowDownIcon,
  BlockedInfoRowStats,
  BlockedInfoRowText,
  Toggle,
  LinkAction
} from '../../../../components'

// Group Components
import NoScriptOverlay from '../overlays/noScriptOverlay'

// Fake data
import { getLocale } from '../../../fakeLocale'
import data from '../../../fakeData'

// Helpers
import { getTabIndexValueBasedOnProps } from '../../../helpers'

interface Props {
  favicon: string
  hostname: string
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  scriptsBlocked: number
}

interface State {
  scriptsBlockedOpen: boolean
  scriptsBlockedEnabled: boolean
}

export default class ScriptsControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      scriptsBlockedOpen: false,
      scriptsBlockedEnabled: true
    }
  }

  get tabIndex () {
    const { isBlockedListOpen, scriptsBlocked } = this.props
    return getTabIndexValueBasedOnProps(isBlockedListOpen, scriptsBlocked)
  }

  onOpenScriptsBlockedOpen = (event: React.MouseEvent<HTMLDivElement>) => {
    if (event.currentTarget) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ scriptsBlockedOpen: !this.state.scriptsBlockedOpen })
  }

  onOpenScriptsBlockedOpenViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event) {
      if (event.key === ' ') {
        event.currentTarget.blur()
        this.props.setBlockedListOpen()
        this.setState({ scriptsBlockedOpen: !this.state.scriptsBlockedOpen })
      }
    }
  }

  onChangeScriptsBlockedEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ scriptsBlockedEnabled: event.target.checked })
  }

  onAllowScriptsOnceViaShortcut = () => {
    return
  }

  render () {
    const { favicon, hostname, isBlockedListOpen, scriptsBlocked } = this.props
    const { scriptsBlockedEnabled, scriptsBlockedOpen } = this.state
    return (
      <>
        <BlockedInfoRow extraColumn={true}>
          <BlockedInfoRowData
            disabled={scriptsBlocked === 0}
            tabIndex={this.tabIndex}
            onClick={this.onOpenScriptsBlockedOpen}
            onKeyDown={this.onOpenScriptsBlockedOpenViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats>{scriptsBlocked > 99 ? '99+' : scriptsBlocked}</BlockedInfoRowStats>
            <BlockedInfoRowText>
              <span>{getLocale('scriptsBlocked')}</span>
            </BlockedInfoRowText>
          </BlockedInfoRowData>
          <LinkAction size='small' style={{ zIndex: 1 }} onClick={this.onAllowScriptsOnceViaShortcut}>
            {getLocale('allowScriptsOnce')}
          </LinkAction>
          <Toggle
            size='small'
            disabled={isBlockedListOpen}
            checked={scriptsBlockedEnabled}
            onChange={this.onChangeScriptsBlockedEnabled}
          />
        </BlockedInfoRow>
        {
          scriptsBlockedOpen &&
            <NoScriptOverlay
              favicon={favicon}
              hostname={hostname}
              name={getLocale('scriptsOnThisSite')}
              list={data.blockedScriptsResources}
              onClose={this.onOpenScriptsBlockedOpen}
            />
        }
      </>
    )
  }
}
