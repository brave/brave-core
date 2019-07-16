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
  LinkAction,
  Toggle
} from '../../../components'

// Group Components
import NoScript from '../overlays/noScriptOverlay'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Helpers
import {
  shouldDisableResourcesRow,
  blockedResourcesSize,
  maybeBlockResource,
  getTabIndexValueBasedOnProps,
  getToggleStateViaEventTarget
} from '../../../helpers/shieldsUtils'

// Types
import { BlockJSOptions } from '../../../types/other/blockTypes'
import { NoScriptInfo } from '../../../types/other/noScriptInfo'
import {
  BlockJavaScript,
  AllowScriptOriginsOnce,
  SetScriptBlockedCurrentState,
  SetGroupedScriptsBlockedCurrentState,
  SetAllScriptsBlockedCurrentState,
  SetFinalScriptsBlockedState
} from '../../../types/actions/shieldsPanelActions'

interface CommonProps {
  // Global props
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  hostname: string
  favicon: string
}

interface JavaScriptProps {
  javascript: BlockJSOptions
  javascriptBlocked: number
  noScriptInfo: NoScriptInfo
  blockJavaScript: BlockJavaScript
  allowScriptOriginsOnce: AllowScriptOriginsOnce
  setScriptBlockedCurrentState: SetScriptBlockedCurrentState
  setGroupedScriptsBlockedCurrentState: SetGroupedScriptsBlockedCurrentState
  setAllScriptsBlockedCurrentState: SetAllScriptsBlockedCurrentState
  setFinalScriptsBlockedState: SetFinalScriptsBlockedState
}

export type Props = CommonProps & JavaScriptProps

interface State {
  scriptsBlockedOpen: boolean
}

export default class ScriptsControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { scriptsBlockedOpen: false }
  }

  get shouldDisableResourcesRow (): boolean {
    const { javascriptBlocked } = this.props
    return shouldDisableResourcesRow(javascriptBlocked)
  }

  get javascriptBlockedDisplay (): string {
    const { javascriptBlocked } = this.props
    return blockedResourcesSize(javascriptBlocked)
  }

  get tabIndex (): number {
    const { isBlockedListOpen, javascriptBlocked } = this.props
    return getTabIndexValueBasedOnProps(isBlockedListOpen, javascriptBlocked)
  }

  get shouldBlockJavaScript (): boolean {
    const { javascript } = this.props
    return maybeBlockResource(javascript)
  }

  triggerOpenScriptsBlocked = (
    event: React.MouseEvent<HTMLDivElement> | React.KeyboardEvent<HTMLDivElement>
  ) => {
    if (event) {
      event.currentTarget.blur()
    }
    this.props.setBlockedListOpen()
    this.setState({ scriptsBlockedOpen: !this.state.scriptsBlockedOpen })
  }

  onOpenScriptsBlocked = (event: React.MouseEvent<HTMLDivElement>) => {
    this.triggerOpenScriptsBlocked(event)
  }

  onOpenScriptsBlockedViaKeyboard = (event: React.KeyboardEvent<HTMLDivElement>) => {
    if (event.key === ' ') {
      this.triggerOpenScriptsBlocked(event)
    }
  }

  onChangeScriptsBlockedEnabled = (event: React.ChangeEvent<HTMLInputElement>) => {
    const shouldBlockJavaScript = getToggleStateViaEventTarget(event)
    this.props.blockJavaScript(shouldBlockJavaScript)
  }

  onClickAllowScriptsOnce = () => {
    this.props.setAllScriptsBlockedCurrentState(false)
    this.props.setFinalScriptsBlockedState()
    this.props.allowScriptOriginsOnce()
  }

  render () {
    const {
      favicon,
      hostname,
      isBlockedListOpen,
      allowScriptOriginsOnce,
      noScriptInfo,
      setScriptBlockedCurrentState,
      setGroupedScriptsBlockedCurrentState,
      setAllScriptsBlockedCurrentState,
      setFinalScriptsBlockedState
    } = this.props
    const { scriptsBlockedOpen } = this.state
    return (
      <>
        <BlockedInfoRow id='scriptsControl' extraColumn={true}>
          <BlockedInfoRowData
            disabled={this.shouldDisableResourcesRow}
            tabIndex={this.tabIndex}
            onClick={this.onOpenScriptsBlocked}
            onKeyDown={this.onOpenScriptsBlockedViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats id='blockScriptsStat'>{this.javascriptBlockedDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('scriptsBlocked')}</BlockedInfoRowText>
          </BlockedInfoRowData>
          {
            this.shouldDisableResourcesRow === false
              && (
                <LinkAction
                  size='small'
                  onClick={this.onClickAllowScriptsOnce}
                  style={{
                    // TODO: cezaraugusto re-visit shields components.
                    // this should be defined in the component itself and not inlined,
                    // and ideally in a logic that is not bounded to a reusable component such as this one.
                    zIndex: 1
                  }}
                >
                  {getLocale('allowScriptsOnce')}
                </LinkAction>
              )
          }
          <Toggle
            id='blockScripts'
            size='small'
            disabled={isBlockedListOpen}
            checked={this.shouldBlockJavaScript}
            onChange={this.onChangeScriptsBlockedEnabled}
          />
        </BlockedInfoRow>
        {
          scriptsBlockedOpen &&
            <NoScript
              favicon={favicon}
              hostname={hostname}
              noScriptInfo={noScriptInfo}
              onClose={this.onOpenScriptsBlocked}
              allowScriptOriginsOnce={allowScriptOriginsOnce}
              setScriptBlockedCurrentState={setScriptBlockedCurrentState}
              setGroupedScriptsBlockedCurrentState={setGroupedScriptsBlockedCurrentState}
              setAllScriptsBlockedCurrentState={setAllScriptsBlockedCurrentState}
              setFinalScriptsBlockedState={setFinalScriptsBlockedState}
            />
        }
      </>
    )
  }
}
