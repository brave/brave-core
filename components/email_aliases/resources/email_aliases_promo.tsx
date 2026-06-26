
// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import * as React from 'react'
import { StyleSheetManager } from 'styled-components'
import styled from 'styled-components'
import { setIconBasePath } from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { color, font, spacing } from '@brave/leo/tokens/css/variables'
import {
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesPromoHandlerInterface,
  EmailAliasesPromoHandler,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import Col from './content/styles/Col'
import Row from './content/styles/Row'

const PromoContainer = styled(Col)`
  height: 100%;
  min-height: 0;
`

const PromoHeader = styled(Row)`
  width: 100%;
  justify-content: space-between;
  padding: ${spacing.m};
  border-bottom: 1px solid ${color.divider.subtle};
  box-sizing: border-box;
  flex-shrink: 0;
`

const PromoTitle = styled.h4`
  color: ${color.text.primary};
  font: ${font.heading.h4};
  margin: 0;
`

const PromoContent = styled.div`
  flex: 1;
  padding: ${spacing.m};
  overflow-y: auto;
  color: ${color.text.primary};
  font: ${font.default.regular};
`

const PromoFooter = styled(Row)`
  justify-content: flex-end;
  padding: ${spacing.m};
  border-top: 1px solid ${color.divider.subtle};
  flex-shrink: 0;
  & leo-button {
    flex-grow: 0;
  }
`

const EmailAliasesPromo = ({
  onClose,
  onGetStarted,
}: {
  onClose: () => void
  onGetStarted: () => void
}) => {
  return (
    <PromoContainer>
      <PromoHeader>
        <PromoTitle>Email Aliases</PromoTitle>
        <Button
          kind='plain'
          onClick={onClose}
        >
          <Icon name='close' />
        </Button>
      </PromoHeader>
      <PromoContent>
        <p>
          Protect your real email address with Brave Email Aliases. Create
          anonymous email addresses that forward messages to your real inbox —
          without ever revealing your identity to senders.
        </p>
      </PromoContent>
      <PromoFooter>
        <Button
          kind='filled'
          onClick={onGetStarted}
        >
          Get started
        </Button>
      </PromoFooter>
    </PromoContainer>
  )
}

export const EmailAliasesPromoConnected = ({
  emailAliasesPromoHandler,
  bindObserver,
}: {
  emailAliasesPromoHandler: EmailAliasesPromoHandlerInterface
  bindObserver: (observer: EmailAliasesServiceObserverInterface) => () => void
}) => {
  return (
    <EmailAliasesPromo
      onClose={() => emailAliasesPromoHandler.onPromoClosed()}
      onGetStarted={() => emailAliasesPromoHandler.onPromoClosed()}
    />
  )
}

const mount = () => {
  const rootElement = document.getElementById('mountPoint')!
  const emailAliasesPromoHandler = EmailAliasesPromoHandler.getRemote()
  const bindObserver = (observer: EmailAliasesServiceObserverInterface) => {
    const observerReceiver = new EmailAliasesServiceObserverReceiver(observer)
    return () => observerReceiver.$.close()
  }
  setIconBasePath('//resources/brave-icons')
  createRoot(rootElement).render(
    <StyleSheetManager>
      <EmailAliasesPromoConnected
        emailAliasesPromoHandler={emailAliasesPromoHandler}
        bindObserver={bindObserver}
      />
    </StyleSheetManager>,
  )
}

document.addEventListener('DOMContentLoaded', mount)