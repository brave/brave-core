// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, Link, NavLink } from 'react-router-dom'

// utils
import { getLocale } from '$web-common/locale'

// routes
import { WalletRoutes } from '../../../constants/types'

// style
import { LinkText } from '../../../components/shared/style'
import { BackIcon } from '../../buy-send-swap/select-header/style'
import { BackButton, DotsWrapper, FlexBox, Wrapper } from './steps-navigation.style'

export interface StepsNavigationProps<T extends string> {
  currentStep: T
  goBack?: () => void
  goBackUrl?: WalletRoutes
  onSkip?: () => void
  preventSkipAhead?: boolean
  preventGoBack?: boolean
  readonly steps: T[]
  skipButtonText?: React.ReactNode
}

export const StepsNavigation: <T extends string>(
  props: StepsNavigationProps<T>
) => JSX.Element = ({
  currentStep,
  goBack,
  goBackUrl,
  onSkip,
  preventSkipAhead,
  steps,
  skipButtonText,
  preventGoBack
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
  }, [goBackUrl, goBack, history])

  return (
    <Wrapper>
      {preventGoBack
        ? <FlexBox />
        : <BackButton
            as={buttonProps.as}
            to={buttonProps.to}
            onClick={buttonProps.onClick}
          >
            <BackIcon />
            <span>{getLocale('braveWalletBack')}</span>
          </BackButton>
      }

      <DotsWrapper>
        {steps.map((stepName, stepIndex) => {
          const isCurrentStep = currentStep === stepName

          const isAvailableNextStep =
            !preventSkipAhead &&
            stepIndex > currentStepIndex

          const isAvailablePrevStep =
            !preventGoBack &&
            stepIndex < currentStepIndex

          const showLink =
            isCurrentStep ||
            isAvailableNextStep ||
            isAvailablePrevStep

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

      {onSkip
        ? <LinkText onClick={onSkip}>
            {skipButtonText || getLocale('braveWalletButtonSkip')}
          </LinkText>
        : <FlexBox />
      }

    </Wrapper>
  )
}

export default StepsNavigation
