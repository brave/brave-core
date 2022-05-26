// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, Link, NavLink } from 'react-router-dom'

// utils
import { getLocale } from '$web-common/locale'

// routes
import { WalletRoutes } from 'components/brave_wallet_ui/constants/types'

// style
import { BackIcon } from '../../buy-send-swap/select-header/style'
import { BackButton, DotsWrapper, FlexBox, Wrapper } from './steps-navigation.style'

export interface StepsNavigationProps<T extends string> {
  readonly steps: T[]
  currentStep: T
  goBackUrl?: WalletRoutes
  goBack?: () => void
  preventSkipAhead?: boolean
}

export const StepsNavigation: <T extends string>(
  props: StepsNavigationProps<T>
) => JSX.Element = ({
  goBack,
  goBackUrl,
  preventSkipAhead,
  currentStep,
  steps
}) => {
  // routing
  const history = useHistory()

  // computed
  const currentStepIndex = steps.findIndex(s => s === currentStep)

  // memos
  const buttonProps = React.useMemo(() => {
    return goBackUrl
      ? { as: Link, to: goBackUrl } as const
      : { onClick: goBack || history.goBack, as: 'button' } as const
  }, [goBackUrl])

  return (
    <Wrapper>
      <BackButton
        as={buttonProps.as}
        to={buttonProps.to}
        onClick={buttonProps.onClick}
      >
        <BackIcon />
        {getLocale('braveWalletBack')}
      </BackButton>

      <DotsWrapper>
        {steps.map((stepName, stepIndex) => {
          const showLink = stepIndex > currentStepIndex
            ? !preventSkipAhead
            : true

          return showLink
            ? <NavLink
                to={stepName}
                key={stepName}
                isActive={() => currentStep === stepName}
                activeClassName="active"
              />
            : <div key={stepName} />
        })}
      </DotsWrapper>

      <FlexBox />

    </Wrapper>
  )
}

export default StepsNavigation
