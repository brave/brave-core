// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import {
    SweepStakesBannerIllustration,
    LearnMoreLink,
    FooterRightColumn,
    BodyText,
    Title,
    Card,
    CloseButton,
    FooterLeftColumn,
    TitleAndCloseRow,
    Footer
} from './style'

import SwapStakesBAYC from '../../../assets/png-icons/swapstakes-bayc.png'
import { getLocale } from '../../../../common/locale'
import { VisibleOnlyDuringTimeFrame } from '../../shared/visible-only-during-timeframe'
import { LocaleRestricted } from '../../shared/locale-restricted'

interface Props {
    // for testing + storybook
    startDate?: Date
    endDate?: Date
}

/**
 * Key for localStorage object that records if the user has closed the sweepstakes banner.
*/
const STORAGE_KEY_SWAP_SWEEPSTAKES_CLOSED = 'BRAVE_WALLET_SWAP_SWEEPSTAKES_BANNER_IS_CLOSED'

const START_DATE = new Date('2022-03-07T00:01:00-0800') // 12:01am PST on March 7, 2022 (1 day before event)
const END_DATE = new Date('2022-03-15T00:00:00-0800') // 12:00am PST March 8, 2022

const LEARN_MORE_LINK = 'https://brave.com/sweepstakes/'

export const SweepstakesBanner = ({
    endDate = END_DATE,
    startDate = START_DATE
}: Props) => {
    const [isVisible, setIsVisible] = React.useState(
        // closed by user?
        window.localStorage.getItem(STORAGE_KEY_SWAP_SWEEPSTAKES_CLOSED) !== 'true'
    )

    const onCloseBanner = () => {
        window.localStorage.setItem(STORAGE_KEY_SWAP_SWEEPSTAKES_CLOSED, 'true')
    }

    return isVisible ? (
        <LocaleRestricted allowedLocales={['en-US']}>
            <VisibleOnlyDuringTimeFrame startDate={startDate} endDate={endDate}>
                <Card>

                    <TitleAndCloseRow>
                        <Title>
                            {/* Swap Sweepstakes */}
                            {getLocale('braveWalletSweepstakesTitle')}
                        </Title>

                        <CloseButton onClick={() => {
                            onCloseBanner()
                            setIsVisible(false)
                        }} />
                    </TitleAndCloseRow>

                    <Footer>
                        <FooterLeftColumn>
                            <BodyText>
                                {getLocale('braveWalletSweepstakesDescription')}
                            </BodyText>
                            <LearnMoreLink href={LEARN_MORE_LINK}>
                                {getLocale('braveWalletSweepstakesCallToAction')}
                            </LearnMoreLink>
                        </FooterLeftColumn>

                        <FooterRightColumn>
                            <SweepStakesBannerIllustration src={SwapStakesBAYC} />
                        </FooterRightColumn>
                    </Footer>

                </Card>
            </VisibleOnlyDuringTimeFrame>
        </LocaleRestricted>
    ) : null
}
