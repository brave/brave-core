/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import PsstProgressModal from '../components/PsstProgressModal'
import { SettingItem } from '../components/SettingsCard';
import { closeDialog } from '../browser_proxy';


interface Props {
    someProp: string
}

export default class PsstDlgContainer extends React.Component<Props, {}> {
    constructor(props: Props) {
        super(props)
    }

    render() {

const settingsItems: SettingItem[] = [
    { id: "id_0", val: 'Disable attaching location information to posts.'},
    { id: "id_1", val: "Disable sharing additional information with X's business partners." },
    { id: "id_2", val: 'Disable personalization based on your inferred identity.' },
    { id: "id_3", val: 'Disable personalized ads.' },
  ];
  
        return (<PsstProgressModal 
                titleText={'Privacy settings optimization'} 
                descriptionText='This website offers privacy settings that Brave can automatically adjust for you. Here is what we recommend:' onSubmitReport={function (): void {
            throw new Error('Function not implemented.' + this.props.someProp);
        } } settingItems={settingsItems} onClose={() => {
            console.log('[PSST] closeDialog')
            closeDialog() 
        } }/>)
    }
}