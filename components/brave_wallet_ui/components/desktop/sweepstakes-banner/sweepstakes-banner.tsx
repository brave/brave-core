import * as React from 'react'
import {
    SweepStakesBannerIllustration,
    LearnMoreLink,
    SweepstakesBannerLeftColumn,
    SweepstakesBannerRightColumn,
    SweepstakesBannerText,
    SweepstakesBannerTitle,
    SweepstakesBannerWrapper
} from './style'

import GiftsIllustration from '../../../assets/svg-illustrations/gifts.svg'
import { CloseButton } from '../popup-modals/style'
import { getLocale } from '../../../../common/locale'

interface Props {
    startDate: Date
    endDate: Date
}

export const SweepstakesBanner = ({ endDate, startDate }: Props) => {
    const now = new Date()
    const [isVisible, setIsVisible] = React.useState(now >= startDate && now <= endDate)

    return isVisible ? <SweepstakesBannerWrapper>

        <SweepstakesBannerLeftColumn>
            <SweepstakesBannerTitle>
                {/* Swap Sweepstakes */}
                {getLocale('braveWalletSweepstakesTitle')}
            </SweepstakesBannerTitle>
            <SweepstakesBannerText>
                {/* Chance to win a bored ape and other cool prizes */}
                {getLocale('braveWalletSweepstakesDescription')}
            </SweepstakesBannerText>
            <LearnMoreLink href='TODO'>
                {/* Learn more (braveWalletWelcomePanelButton) */}
                {getLocale('braveWalletWelcomePanelButton')}
            </LearnMoreLink>
        </SweepstakesBannerLeftColumn>

        <SweepstakesBannerRightColumn>
            <CloseButton onClick={() => {
                setIsVisible(false)
            }} />
            <SweepStakesBannerIllustration src={GiftsIllustration} />
        </SweepstakesBannerRightColumn>

    </SweepstakesBannerWrapper> : null
}
