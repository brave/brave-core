/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { ClientContext, useClientListener } from '../lib/client_context'
import { Client } from '../lib/interfaces'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { BraveTalkOptInForm } from '../../shared/components/onboarding/brave_talk_opt_in_form'

interface Props {
  client: Client
}

export function App (props: Props) {
  const { client } = props

  const [loading, setLoading] = React.useState(client.state.loading)
  const [showRewardsOnboarding, setShowRewardsOnboarding] =
    React.useState(client.state.showRewardsOnboarding)

  useClientListener(props.client, (state) => {
    setLoading(state.loading)
    setShowRewardsOnboarding(state.showRewardsOnboarding)
  })

  if (loading) {
    return null
  }

  return (
    <ClientContext.Provider value={props.client}>
      <LocaleContext.Provider value={props.client}>
        <WithThemeVariables>
          <BraveTalkOptInForm
            showRewardsOnboarding={showRewardsOnboarding}
            onEnable={client.enableAds}
            onTakeTour={client.openRewardsTour}
          />
        </WithThemeVariables>
      </LocaleContext.Provider>
    </ClientContext.Provider>
  )
}
