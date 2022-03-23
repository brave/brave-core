import styled from 'styled-components'
import ShareIcon from '../../../../../../assets/svg-icons/share.svg'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
`

export const AssetDescriptionWrapper = styled.div`
  margin-top: 6px;
`

export const AssetDescription = styled.p`
  font-family: Poppins;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text02};
  margin: 0;
`

export const AssetStatsWrapper = styled.div`
  display: flex;
  justify-content: flex-start;
  align-items: center;
  margin: 12px 0;
`

export const AssetStatWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 11px;
`

export const AssetStatSpacer = styled.div`
  display: flex;
  width: 95px;
`

export const AssetStat = styled.span`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-style: normal;
  font-weight: 500;
  font-size: 24px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text01};
`

export const AssetStatLabel = styled.span`
  display: flex;
  align-items: center;
  text-align: center;
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text02};
`

export const AssetLink = styled.a`
  display: inline-flex;
  align-items: center;
  text-align: center;
  font-family: Poppins;
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  text-decoration: none;
  margin-right: 8px;
`

export const LinkIcon = styled.span`
  display: inline-flex;
  width: 16px;
  height: 14px;
  background-color: ${(p) => p.theme.color.interactive05};
  -webkit-mask-image: url(${ShareIcon});
  mask-image: url(${ShareIcon});
`

export const Row = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const Sup = styled.sup`
 font-family: Poppins;
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  padding-right: 2px;
  color: ${p => p.theme.color.text02};
`
