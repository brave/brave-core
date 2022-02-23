import styled from 'styled-components'

export const SweepstakesBannerWrapper = styled.div`
    display: flex;
    flex-direction: row;
    width: 100%;
    background: ${(p) => p.theme.color.background01};
    border-radius: 8px;
    /* Light Theme/Dividers and outlines/divider01 */
    border: 2px solid ${(p) => p.theme.color.divider01};
`

export const SweepstakesBannerLeftColumn = styled.div`
    display: flex;
    flex: 1;
    flex-direction: column;
    padding-top: 16px;
    padding-left: 16px;
`

export const SweepstakesBannerRightColumn = styled.div`
    display: flex;
    flex: 1;
    flex-direction: column;
    align-items: flex-end;
    justify-content: space-between;
    padding-top: 12px;
    padding-right: 12px;
`

export const SweepstakesBannerTitle = styled.h2`
    font-family: Poppins;
    font-style: normal;
    font-weight: 600;
    font-size: 18px;
    line-height: 26px;
    margin-top: 0px;
    margin-bottom: 0px;
    /* Light Theme/Text/text01 */
    color: ${(p) => p.theme.color.text01};
`

export const SweepstakesBannerText = styled.span`
    font-family: Poppins;
    font-style: normal;
    font-weight: normal;
    font-size: 12px;
    line-height: 18px;
    letter-spacing: 0.01em;
    /* Light Theme/Text/text01 */
    color: ${(p) => p.theme.color.text01};
`

export const LearnMoreLink = styled.a`
    font-family: Poppins;
    font-style: normal;
    font-weight: 600;
    font-size: 12px;
    line-height: 20px;
    text-decoration: none;
    /* Light Theme/Brand/interactive05 */
    color: ${(p) => p.theme.color.interactive05};
    
    display: flex;
    flex-direction: column;
    justify-content: center;
    height: 40px;
`

export const SweepStakesBannerIllustration = styled.img`
    width: 60px;
    height: 36px;
    position: relative;
    right: 26px;
`
