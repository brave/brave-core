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
  Toggle
} from 'brave-ui/features/shields'

// Group Components
import DynamicList from '../list/dynamic'

// Locale
import { getLocale } from '../../background/api/localeAPI'

// Helpers
import {
  maybeDisableResourcesRow,
  blockedResourcesSize,
  maybeBlockResource,
  getTabIndexValueBasedOnProps,
  getToggleStateViaEventTarget
} from '../../helpers/shieldsUtils'

// Types
import { BlockJSOptions } from '../../types/other/blockTypes'
import { NoScriptInfo } from '../../types/other/noScriptInfo'
import {
  ChangeNoScriptSettings,
  BlockJavaScript,
  ChangeAllNoScriptSettings,
  AllowScriptOriginsOnce,
  SetFinalScriptsBlockedState
} from '../../types/actions/shieldsPanelActions'

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
  changeNoScriptSettings: ChangeNoScriptSettings
  blockJavaScript: BlockJavaScript
  changeAllNoScriptSettings: ChangeAllNoScriptSettings
  allowScriptOriginsOnce: AllowScriptOriginsOnce
  setFinalScriptsBlockedState: SetFinalScriptsBlockedState
}

export type Props = CommonProps & JavaScriptProps

interface State {
  scriptsBlockedOpen: boolean
}

export default class ScriptsControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      scriptsBlockedOpen: false
    }
  }

  get maybeDisableResourcesRow (): boolean {
    const { javascriptBlocked } = this.props
    return maybeDisableResourcesRow(javascriptBlocked)
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
    event.currentTarget.blur()
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

  render () {
    const {
      favicon,
      hostname,
      isBlockedListOpen,
      allowScriptOriginsOnce,
      changeNoScriptSettings,
      changeAllNoScriptSettings,
      setFinalScriptsBlockedState,
      noScriptInfo
    } = this.props
    const { scriptsBlockedOpen } = this.state
    return (
      <>
        <BlockedInfoRow id='scriptsControl'>
          <BlockedInfoRowData
            disabled={this.maybeDisableResourcesRow}
            tabIndex={this.tabIndex}
            onClick={this.onOpenScriptsBlocked}
            onKeyDown={this.onOpenScriptsBlockedViaKeyboard}
          >
            <ArrowDownIcon />
            <BlockedInfoRowStats id='blockScriptsStat'>{this.javascriptBlockedDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('scriptsBlocked')}</BlockedInfoRowText>
          </BlockedInfoRowData>
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
            <DynamicList
              favicon={favicon}
              hostname={hostname}
              origin={origin}
              name={getLocale('scriptsOnThisSite')}
              list={noScriptInfo}
              onClose={this.onOpenScriptsBlocked}
              allowScriptOriginsOnce={allowScriptOriginsOnce}
              changeNoScriptSettings={changeNoScriptSettings}
              changeAllNoScriptSettings={changeAllNoScriptSettings}
              setFinalScriptsBlockedState={setFinalScriptsBlockedState}
            />
        }
      </>
    )
  }
}
