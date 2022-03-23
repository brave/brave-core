import * as React from 'react'
import { BraveWallet } from '../../../../../../constants/types'
import { DividerText, Spacer, SubDivider } from '../portfolio-top-section/style'
import { formatPriceWithAbbreviation } from '../../../../../../utils/format-prices'
import { getLocale } from '../../../../../../../common/locale'
import {
  AssetStat,
  AssetStatLabel,
  AssetStatSpacer,
  AssetStatsWrapper,
  AssetStatWrapper,
  Row,
  StyledWrapper,
  Sup
} from './style'

interface Props {
  selectedCoinMarket: BraveWallet.CoinMarket
}

const AssetStats = (props: Props) => {
  const { selectedCoinMarket } = props
  const { marketCap, marketCapRank, totalVolume } = selectedCoinMarket
  const formattedMarketCap = formatPriceWithAbbreviation(marketCap.toString(), '', 2)
  const formattedVolume = formatPriceWithAbbreviation(totalVolume.toString(), '', 2)

  return (
    <StyledWrapper>
      <Spacer />
      <DividerText>
        {getLocale('braveWalletMarketDataDetailInformation')}
      </DividerText>
      <SubDivider />
      {/* Asset decription will be added when data is available from Api */}
      {/* <AssetDescriptionWrapper>
        <AssetDescription></AssetDescription>
      </AssetDescriptionWrapper> */}
      <AssetStatsWrapper>
        <AssetStatWrapper>
          <AssetStat>{marketCapRank}</AssetStat>
          <AssetStatLabel>
            {getLocale('braveWalletMarketDataDetailRank')}
          </AssetStatLabel>
        </AssetStatWrapper>
        <AssetStatSpacer />
        <AssetStatWrapper>
          <Row>
            <Sup>$</Sup>
            <AssetStat>
              {formattedVolume}
            </AssetStat>
          </Row>
          <AssetStatLabel>
            {getLocale('braveWalletMarketDataDetailVolume')}
          </AssetStatLabel>
        </AssetStatWrapper>
        <AssetStatSpacer />
        <AssetStatWrapper>
          <Row>
            <Sup>$</Sup>
            <AssetStat>
              {formattedMarketCap}
            </AssetStat>
          </Row>
          <AssetStatLabel>
            {getLocale('braveWalletMarketDataDetailMarketCap')}
          </AssetStatLabel>
        </AssetStatWrapper>
        <AssetStatSpacer />
        {/* Asset website. Currently data not available from API */}
        {/* <AssetStatWrapper>
          <AssetStat>
            {getLocale('braveWalletMarketDataDetailWebsite')}
          </AssetStat>
          <Row>
            <AssetLink href="https://ethereum.org">www.ethereum.org</AssetLink>
            <LinkIcon />
          </Row>
        </AssetStatWrapper> */}
      </AssetStatsWrapper>
    </StyledWrapper>
  )
}

export default AssetStats
