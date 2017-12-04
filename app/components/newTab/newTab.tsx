/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 import * as React from 'react'
 import {getMessage} from '../../background/api/localeAPI'

 export interface Props {
   actions: {
     settingsIconClicked: () => {}
   }
 }

 class NewTab extends React.Component<Props, object> {
    render() {
        const { actions } = this.props

        return (
        <div data-test-id='new-tab-page'>
            <div>Hello, {getMessage('newTab')} world!</div>
            <div onClick={actions.settingsIconClicked}>Settings</div>
        </div>
        )
    }
 }

 export default NewTab

