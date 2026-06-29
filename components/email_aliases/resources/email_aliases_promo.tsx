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
import { color, font, spacing, radius, typography } from '@brave/leo/tokens/css/variables'
import {
    EmailAliasesServiceObserverInterface,
    EmailAliasesServiceObserverReceiver,
    EmailAliasesPromoHandlerInterface,
    EmailAliasesPromoHandler,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import Col from './content/styles/Col'
import Row from './content/styles/Row'
import NytIcon from './content/assets/nyt-logo.svg'
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
  line-height: ${typography.heading.h4.lineHeight};
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
  color: ${color.text.tertiary};
`

const GetStartedButton = styled(Button)`
  align-self: flex-end;
`

// ---------------------------------------------------------------------------
// Illustration cards
// ---------------------------------------------------------------------------

const IllustrationArea = styled(Col)`
  align-items: center;
  gap: ${spacing.s};
`

const CardsRow = styled(Row)`
  align-items: center;
  justify-content: center;
  width: 100%;
  position: relative;
`

const SiteCard = styled(Col) <{ $faded?: boolean }>`
  border-radius: ${radius.m};
  border: ${spacing.xs} solid ${color.divider.subtle};
  padding: ${spacing.s};
  gap: ${spacing.s};
  background: ${color.container.background};
  opacity: ${({ $faded }) => ($faded ? 0.45 : 1)};
`

const SiteCardFocused = styled(SiteCard)`
  flex: 1.4;
  opacity: 1;
  gap: ${spacing.l};
  z-index: 1;
  position: relative;
  /* Pull the side cards underneath the focused card's edges. */
  margin: 0 calc(-1 * ${spacing['2Xl']});
  /* Extra vertical padding makes the focused card taller than its
     neighbors so it stands out in front. */
  padding: ${spacing['2Xl']} ${spacing['2Xl']};
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.16);
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

const AliasChipFocused = styled(AliasChip)`
  padding: ${spacing.m} ${spacing.l};
  text-align: center;
  background: ${color.container.background};
`

const LabelChipGroup = styled(Col)`
  gap: ${spacing.s};
`

const AliasPart = styled.span`
  color: #7c3aed;
`

const AliasNumber = styled.span`
  color: ${color.systemfeedback.errorText};
`

// ---------------------------------------------------------------------------
// Arrow
// ---------------------------------------------------------------------------

const ArrowWrapper = styled(Col)`
  align-items: center;
  color: ${color.systemfeedback.errorText};
  line-height: ${spacing.xs};
`

const ArrowLine = styled.div`
  width: ${spacing.s};
  height: ${spacing.xl};
  background: ${color.systemfeedback.errorText};
`

const ArrowHead = styled.div`
  width: ${spacing.none};
  height: ${spacing.none};
  border-left: ${spacing.s} solid transparent;
  border-right: ${spacing.s} solid transparent;
  border-top: ${spacing.m} solid ${color.systemfeedback.errorText};
`

// ---------------------------------------------------------------------------
// Real-email row
// ---------------------------------------------------------------------------

const RealEmailRow = styled(Row)`
  align-items: center;
  gap: ${spacing.s};
  border-radius: ${radius.m};
  border: 1px solid ${color.divider.subtle};
  padding: ${spacing.m} ${spacing.xl};
  background: ${color.container.background};
  box-sizing: border-box;
  color: ${color.text.secondary};
  font: ${font.default.regular};
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
                    <Button fab kind='plain-faint' onClick={onClose}>
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
                            <SiteTitle style={{ color: '#0071ce' }}>Walmart ✦</SiteTitle>
                            <Label>Register on Wal…</Label>
                            <AliasChip>
                                <AliasPart>lion.paw.</AliasPart>
                                <AliasNumber>75</AliasNumber>
                                <span>…</span>
                            </AliasChip>
                        </SiteCard>

                        {/* Center — focused NYT card */}
                        <SiteCardFocused>
                            <img src={NytIcon} alt="The New York Times" />
                            <LabelChipGroup>
                                <Label>{getLocale(S.EMAIL_ALIASES_PROMO_EMAIL_LABEL)}</Label>
                                <AliasChipFocused>
                                    <AliasPart>cat.mane.</AliasPart>
                                    <AliasNumber>2947</AliasNumber>
                                    <span>@bravealias.com</span>
                                </AliasChipFocused>
                            </LabelChipGroup>
                        </SiteCardFocused>

                        {/* Right — faded generic store card */}
                        <SiteCard $faded>
                            <SiteTitle style={{ color: color.text.secondary }}>
                                store.com
                            </SiteTitle>
                            <Label>&nbsp;</Label>
                            <AliasChip>
                                <span>…ealias.com</span>
                            </AliasChip>
                        </SiteCard>
                    </CardsRow>

                    <ArrowWrapper>
                        <ArrowLine />
                        <ArrowHead />
                    </ArrowWrapper>

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
                <GetStartedButton kind='filled' onClick={onGetStarted}>
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