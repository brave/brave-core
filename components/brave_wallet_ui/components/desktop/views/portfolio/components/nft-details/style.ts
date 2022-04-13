import styled from 'styled-components'
import { WalletButton } from '../../../../../shared/style'
import WebsiteIcon from '../../../../../../assets/svg-icons/website-icon.svg'
import TwitterIcon from '../../../../../../assets/svg-icons/twitter-icon.svg'
import FacebookIcon from '../../../../../../assets/svg-icons/facebook-icon.svg'
import { LoaderIcon, OpenNewIcon } from 'brave-ui/components/icons'

export interface StyleProps {
  isLoading: boolean
}

const nftImageDimension = '440px'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  min-height: ${nftImageDimension};
  margin: 16px 0 50px 0;
`

export const NFTImage = styled.img<Partial<StyleProps>>`
  display: ${p => p.isLoading ? 'none' : 'block'};
  width: ${nftImageDimension};
  height: ${nftImageDimension};
  margin-right: 28px;
  border-radius: 12px;
`

export const NFTImageSkeletonWrapper = styled.div`
  min-width: ${nftImageDimension};
  height: ${nftImageDimension};
  margin-right: 28px;
  border-radius: 12px;
`

export const DetailColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
`

export const TokenName = styled.span`
  font-family: Poppins;
  font-size: 20px;
  line-height: 30px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text01};
  margin-bottom: 15px;
`

export const TokenFiatValue = styled.span`
  font-family: Poppins;
  font-size: 24px;
  line-height: 36px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text01};
`

export const TokenCryptoValue = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-bottom: 20px;
`

export const DetailSectionRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: space-between;
  min-width: 70%;
  margin-bottom: 30px;
`

export const DetailSectionColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
`

export const DetailSectionTitle = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 10px;
`

export const DetailSectionValue = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.04em;
  color: ${(p) => p.theme.color.text01};
  margin-right: 8px;
`

export const ProjectDetailIDRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const ProjectDetailRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  margin-bottom: 20px;
`

export const ProjectDetailName = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 24px;
  font-weight: 500;
  color: ${(p) => p.theme.color.text01};
  margin-right: 12px;
`

export const ProjectDetailDescription = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  margin-right: 12px;
  max-width: 80%;
`

export const ProjectDetailImage = styled.img`
  width: 24px;
  height: 24px;
  margin-right: 5px;
  border-radius: 100%;
`

export const ProjectDetailButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  border: 1px solid #E5E8EB;
  border-radius: 6.5px;
`

export const ProjectDetailButtonSeperator = styled.div`
  display: flex;
  width: 1px;
  height: 32px;
  background-color: #E5E8EB;
`

export const ProjectDetailButton = styled(WalletButton)`
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
`

export const ProjectWebsiteIcon = styled.div`
  width: 14px;
  height: 14px;
  background-color: #8A939B;
  -webkit-mask-image: url(${WebsiteIcon});
  mask-image: url(${WebsiteIcon});
  mask-size: contain;
`

export const ProjectTwitterIcon = styled.div`
  width: 14px;
  height: 14px;
  background-color: #8A939B;
  -webkit-mask-image: url(${TwitterIcon});
  mask-image: url(${TwitterIcon});
  mask-size: contain;
`

export const ProjectFacebookIcon = styled.div`
  width: 14px;
  height: 14px;
  background-color: #8A939B;
  -webkit-mask-image: url(${FacebookIcon});
  mask-image: url(${FacebookIcon});
  mask-size: contain;
`

export const ExplorerButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
`

export const ExplorerIcon = styled(OpenNewIcon)`
  width: 14px;
  height: 14px;
  color: ${(p) => p.theme.color.interactive05};
`

export const LoadingOverlay = styled.div<Partial<StyleProps>>`
  display: ${(p) => p.isLoading ? 'flex' : 'none'};
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 440px;
  position: absolute;
  z-index: 10;
  backdrop-filter: blur(5px);
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: .4;
`
