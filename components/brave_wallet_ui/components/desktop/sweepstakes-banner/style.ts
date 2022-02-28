import styled from 'styled-components'
export { CloseButton } from '../popup-modals/style'

export const Card = styled.div`
    display: flex;
    flex-direction: column;
    width: 100%;
    background: ${(p) => p.theme.color.background01};
    border-radius: 8px;
    border: 2px solid ${(p) => p.theme.color.divider01};
    padding-left: 16px;
    padding-right: 8px;
    margin-top: 16px;
`

const Row = styled.div`
    display: flex;
    flex: 1;
    flex-direction: row;
`

export const TitleAndCloseRow = styled(Row)`
    width: 100%;
    justify-content: space-between;
    padding-top: 8px;
`

export const Title = styled.h2`
    font-family: Poppins;
    font-style: normal;
    font-weight: 600;
    font-size: 18px;
    line-height: 26px;
    margin-top: 4px;
    margin-bottom: 4px;
    word-wrap: no-wrap;
    color: ${(p) => p.theme.color.text01};
`

export const Footer = Row

const Column = styled.div`
    display: flex;
    flex-direction: column;
`

export const BodyText = styled.span`
    font-family: Poppins;
    font-style: normal;
    font-weight: normal;
    font-size: 12px;
    line-height: 18px;
    letter-spacing: 0.01em;
    color: ${(p) => p.theme.color.text01};
`

export const FooterLeftColumn = styled(Column)`
    flex: 1;
`

export const FooterRightColumn = styled(Column)`
    align-items: flex-end;
    justify-content: flex-end;
    padding-right: 24px;
    padding-left: 16px;
`

export const LearnMoreLink = styled.a`
    font-family: Poppins;
    font-style: normal;
    font-weight: 600;
    font-size: 12px;
    line-height: 20px;
    text-decoration: none;
    color: ${(p) => p.theme.color.interactive05};
    display: flex;
    flex-direction: column;
    justify-content: center;
    height: 40px;
`

export const SweepStakesBannerIllustration = styled.img`
    width: 60px;
    height: 36px;
    align-self: flex-end;
`
