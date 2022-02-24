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

import GiftsIllustration from '../../../assets/svg-illustrations/gifts.svg'
import { getLocale } from '../../../../common/locale'
import { VisibleOnlyDuringTimeFrame } from '../../shared/visible-only-during-timeframe'

interface Props {
    // for testing + storybook
    startDate?: Date
    endDate?: Date
}

/**
 * Key for localStorage object that records if the user has closed the sweepstakes banner.
*/
const STORAGE_KEY_SWAP_SWEEPSTAKES_CLOSED = 'BRAVE_WALLET_SWAP_SWEEPSTAKES_BANNER_IS_CLOSED'

// TODO: get dates & link
const START_DATE = new Date()
const END_DATE = new Date()
// Add days
END_DATE.setTime(START_DATE.getTime() + (
    30 * // 30 days from now
    24 * 60 * 60 * 1000 // days in ms
))

const LEARN_MORE_LINK = ''

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
                        {/* Chance to win a bored ape and other cool prizes */}
                        {getLocale('braveWalletSweepstakesDescription')}
                    </BodyText>
                        <LearnMoreLink href={LEARN_MORE_LINK}>
                            {/* Learn more (braveWalletWelcomePanelButton) */}
                            {getLocale('braveWalletWelcomePanelButton')}
                        </LearnMoreLink>
                    </FooterLeftColumn>

                    <FooterRightColumn>
                        <SweepStakesBannerIllustration src={GiftsIllustration} />
                    </FooterRightColumn>
                </Footer>

            </Card>
        </VisibleOnlyDuringTimeFrame>
    ) : null
}
