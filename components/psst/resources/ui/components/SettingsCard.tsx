/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { SettingGrid, SettingGridHeaderRow, SettingGridRow } from './basic/display'
import Checkbox from '@brave/leo/react/checkbox'
import { BravePsstConsentDialogProxy, BravePsstConsentDialogProxyImpl } from '../browser_proxy'

export interface SettingItem {
  id: string
  val: string
}

export interface Props {
  settingItems: SettingItem[]
}

export default class SettingsCard extends React.PureComponent<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  browserProxy_: BravePsstConsentDialogProxy =
  BravePsstConsentDialogProxyImpl.getInstance()

  render () {
    const {
      settingItems
    } = this.props

    this.browserProxy_.getCallbackRouter().onSetRequestDone.addListener((url: string, isError: boolean) => {
      console.log('[PSST] onSetRequestDone url:', url)
    })

    return (
      <SettingGrid>
        <SettingGridHeaderRow>
          <div style={iconContainerStyle}>
            {/* <img
              src="https://abs.twimg.com/favicons/favicon.ico" // Replace with the actual X icon URL
              alt="X Icon"
              style={iconStyle}
            /> */}
          </div>
          <div>
            <div style={titleStyle}>It's what's happening / X</div>
            <div style={urlStyle}>x.com</div>
          </div>
        </SettingGridHeaderRow>
        {settingItems.map((item) => (
          <SettingGridRow key={item.id}>
              <Checkbox checked={true} onChange={() => console.log('checkbox clicked')}>
                  <label>{item.val}</label>
              </Checkbox>
          </SettingGridRow>
        ))
        }
      </SettingGrid>
    )
  }

};


// const headerStyle = {
//   display: 'flex',
//   alignItems: 'center',
//   marginBottom: '16px',
// };

const iconContainerStyle = {
  width: '32px',
  height: '32px',
  borderRadius: '4px',
  backgroundColor: '#fff', // White background for the icon
  display: 'flex',
  justifyContent: 'center',
  alignItems: 'center',
  marginRight: '12px',
};

// const iconStyle = {
//   maxWidth: '20px',
//   maxHeight: '20px',
// };

const titleStyle = {
  fontWeight: 'bold',
  fontSize: '16px',
  color: '#333',
};

const urlStyle = {
  fontSize: '14px',
  color: '#777',
};

// const listItemStyle = {
//   display: 'flex',
//   alignItems: 'center',
//   padding: '16px',
//   borderBottom: '1px solid #eee',
// };

// const checkboxStyle = {
//   width: '24px',
//   height: '24px',  return (

//   borderRadius: '4px',
//   backgroundColor: '#e0e0e0', // Light gray for the background
//   display: 'flex',
//   justifyContent: 'center',
//   alignItems: 'center',
//   marginRight: '12px',
// };

// const checkIconStyle = {
//   color: '#3f51b5', // Blue checkmark color
//   width: '18px',
//   height: '18px',
// };

// const textStyle = {
//   fontSize: '16px',
//   color: '#333',
// };

//export default SettingsCard;