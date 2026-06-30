// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createRoot } from 'react-dom/client'
import * as React from 'react'
import styled, { StyleSheetManager } from 'styled-components'
import Icon, { setIconBasePath } from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import {
  color,
  font,
  spacing,
  radius,
  typography,
} from '@brave/leo/tokens/css/variables'
import {
  EmailAliasesServiceObserverInterface,
  EmailAliasesServiceObserverReceiver,
  EmailAliasesPromoHandlerInterface,
  EmailAliasesPromoHandler,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import Col from './content/styles/Col'
import Row from './content/styles/Row'
import NytIcon from './content/assets/nyt-logo.svg'
import ArrowIcon from './content/assets/arrow.svg'
import { getLocale } from '$web-common/locale'
import './content/strings'

// ---------------------------------------------------------------------------
// Shared modal primitives — mirrors email_aliases_modal.tsx naming
// ---------------------------------------------------------------------------

const ModalCol = styled(Col)`
  row-gap: ${spacing['2Xl']};
`

const Header = styled(Row)`
  width: 100%;
  justify-content: space-between;
  align-items: center;
  box-sizing: border-box;
  flex-shrink: 0;
`

const Title = styled.h4`
  color: ${color.text.secondary};
  font: ${font.heading.h4};
  margin: ${spacing.none};
`
export const RightAlignedItem = styled.div`
  margin-left: auto;
  text-align: right;
`

const Description = styled.p`
  margin: ${spacing.none};
  color: ${color.text.primary};
  font: ${font.default.regular};
  letter-spacing: ${typography.letterSpacing.default};
`

const ContentCol = styled(Col)`
  row-gap: ${spacing['2Xl']};
`

const Label = styled.span`
  font: ${font.small.regular};
  color: ${color.primitive.neutral[0]};
`

const GetStartedButton = styled(Button)`
  align-self: flex-end;
`

// ---------------------------------------------------------------------------
// Illustration cards
// ---------------------------------------------------------------------------

const IllustrationArea = styled(Col)`
  align-items: center;
`

const CardsRow = styled(Row)`
  align-items: center;
  justify-content: center;
  width: 100%;
  position: relative;
`

const SiteCard = styled(Col)<{ $faded?: boolean }>`
  border-radius: ${radius.m};
  border: ${spacing.xs} solid ${color.divider.subtle};
  padding: ${spacing.s};
  gap: ${spacing.s};
  background: ${color.container.background};
  opacity: ${({ $faded }) => ($faded ? 0.45 : 1)};
`

const SiteCardFocused = styled(SiteCard)`
  display: inline-flex;
  padding: ${spacing['2Xl']};
  flex-direction: column;
  align-items: center;
  gap: ${spacing['2Xl']};
  border-radius: ${radius.xl};
  border: 1px solid ${color.primitive.neutral[70]};
  background: ${color.primitive.neutral[100]};
`

const SiteTitle = styled.span`
  font: ${font.default.semibold};
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
`

const AliasChip = styled.div`
  border-radius: ${radius.s};
  border: 1px solid ${color.divider.subtle};
  padding: ${spacing.s} ${spacing.m};
  font: ${font.small.regular};
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  background: ${color.container.highlight};
`

const AliasChipFocused = styled.div`
  display: flex;
  align-items: center;
  padding: ${spacing.m} ${spacing.l};
  align-self: stretch;
  border-radius: ${radius.m};
  border: 1px solid ${color.primitive.neutral[70]};
  background: ${color.primitive.neutral[100]};
`

const LabelChipGroup = styled(Col)`
  gap: ${spacing.none};
`

const AliasPart = styled.span`
  color: ${color.purple[50]};
`

const AliasNumber = styled.span`
  color: ${color.systemfeedback.errorText};
`

// ---------------------------------------------------------------------------
// Arrow
// ---------------------------------------------------------------------------
const ArrowImg = styled.img`
  align-self: center;
  position: relative;
  z-index: 1;
  margin-top: calc(-1 * (${spacing.m} + ${spacing['2Xl']}));
`

// ---------------------------------------------------------------------------
// Real-email row
// ---------------------------------------------------------------------------

const RealEmailRow = styled(Row)`
  display: inline-flex;
  padding: ${spacing.m} ${spacing.xl};
  justify-content: center;
  align-items: center;
  gap: ${spacing.m};
  border-radius: ${radius.l};
  border: 1px solid ${color.primitive.neutral[70]};
  background: ${color.primitive.neutral[100]};
  padding: ${spacing.m} ${spacing.xl};
  box-sizing: border-box;
  color: ${color.primitive.neutral[0]};
  font: ${font.default.semibold};
`

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

const EmailAliasesPromo = ({
  onClose,
  onGetStarted,
}: {
  onClose: () => void
  onGetStarted: () => void
}) => {
  return (
    <ModalCol>
      <Header>
        <Title>{getLocale(S.EMAIL_ALIASES_PROMO_DLG_TITLE)}</Title>
        <RightAlignedItem>
          <Button
            fab
            kind='plain-faint'
            onClick={onClose}
          >
            <Icon name='close' />
          </Button>
        </RightAlignedItem>
      </Header>

      <ContentCol>
        <Description>
          {getLocale(S.EMAIL_ALIASES_PROMO_DLG_TOP_DESCRIPTION)}
        </Description>

        <IllustrationArea>
          <CardsRow>
            {/* Left — faded Walmart card */}
            <SiteCard $faded>
              <SiteTitle style={{ color: `${color.blue[50]}` }}>
                {getLocale(
                  S.EMAIL_ALIASES_PROMO_LEFT_ILLUSTRATION_WINDOW_BRAND,
                )}
              </SiteTitle>
              <Label>
                {getLocale(
                  S.EMAIL_ALIASES_PROMO_LEFT_ILLUSTRATION_WINDOW_BRAND_EMAIL_LABEL,
                )}
              </Label>
              <AliasChip>
                <AliasPart>
                  {getLocale(
                    S.EMAIL_ALIASES_PROMO_LEFT_ILLUSTRATION_WINDOW_EMAIL_PLACEHOLDER,
                  )}
                </AliasPart>
                <AliasNumber>
                  {getLocale(
                    S.EMAIL_ALIASES_PROMO_LEFT_ILLUSTRATION_WINDOW_EMAIL_PLACEHOLDER_SUFFIX,
                  )}
                </AliasNumber>
                <span>…</span>
              </AliasChip>
            </SiteCard>

            {/* Center — focused NYT card */}
            <SiteCardFocused>
              <img
                src={NytIcon}
                alt='The New York Times'
                style={{ alignSelf: 'center' }}
              />
              <LabelChipGroup>
                <Label>{getLocale(S.EMAIL_ALIASES_PROMO_EMAIL_LABEL)}</Label>
                <AliasChipFocused>
                  <AliasPart>
                    {getLocale(
                      S.EMAIL_ALIASES_PROMO_CENTER_ILLUSTRATION_WINDOW_EMAIL_PLACEHOLDER,
                    )}
                  </AliasPart>
                  <AliasNumber>
                    {getLocale(
                      S.EMAIL_ALIASES_PROMO_CENTER_ILLUSTRATION_WINDOW_EMAIL_PLACEHOLDER_SUFFIX,
                    )}
                  </AliasNumber>
                  <span>
                    {getLocale(
                      S.EMAIL_ALIASES_PROMO_CENTER_ILLUSTRATION_WINDOW_EMAIL_PLACEHOLDER_END,
                    )}
                  </span>
                </AliasChipFocused>
              </LabelChipGroup>
            </SiteCardFocused>

            {/* Right — faded generic store card */}
            <SiteCard $faded>
              <SiteTitle style={{ color: color.text.secondary }}>
                {getLocale(
                  S.EMAIL_ALIASES_PROMO_RIGHT_ILLUSTRATION_WINDOW_BRAND,
                )}
              </SiteTitle>
              <Label>&nbsp;</Label>
              <AliasChip>
                <span>
                  {getLocale(
                    S.EMAIL_ALIASES_PROMO_RIGHT_ILLUSTRATION_WINDOW_EMAIL_PLACEHOLDER,
                  )}
                </span>
              </AliasChip>
            </SiteCard>
          </CardsRow>

          <ArrowImg src={ArrowIcon} />

          <RealEmailRow>
            <Icon name='email-shield' />
            {getLocale(S.EMAIL_ALIASES_PROMO_YOUR_EMAIL)}
          </RealEmailRow>
        </IllustrationArea>
        <Description>
          {getLocale(S.EMAIL_ALIASES_PROMO_DLG_BOTTOM_DESCRIPTION)}
        </Description>
      </ContentCol>

      <Col>
        <GetStartedButton
          kind='filled'
          onClick={onGetStarted}
        >
          {getLocale(S.EMAIL_ALIASES_PROMO_DLG_GET_STARTED_BUTTON)}
        </GetStartedButton>
      </Col>
    </ModalCol>
  )
}

// ---------------------------------------------------------------------------
// Connected wrapper
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Mount
// ---------------------------------------------------------------------------

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
