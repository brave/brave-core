import styled from 'styled-components'
import { ArrowRightIcon, LoaderIcon } from 'brave-ui/components/icons'
import { WarningBoxIcon } from '../shared-panel-styles'

import {
  AssetIconProps,
  AssetIconFactory,
  WalletButton
} from '../../shared/style'

interface StyleProps {
  orb: string
  isDetails: boolean
  isApprove: boolean
  needsMargin: boolean
  hasError: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${(p) => p.theme.color.background01};
`

export const TopRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 15px 15px 0px 15px;
`

export const AccountCircleWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  box-sizing: border-box;
  margin-bottom: 10px;
`

export const FromCircle = styled.div<Partial<StyleProps>>`
  width: 54px;
  height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const ToCircle = styled.div<Partial<StyleProps>>`
  width: 32px;
  height: 32px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  position: absolute;
  left: 34px;
`

export const AccountNameText = styled.span`
  cursor: default;
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
`

export const NetworkText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
`

export const TransactionAmountBig = styled.span`
  font-family: Poppins;
  font-size: 18px;
  line-height: 22px;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text01};
  font-weight: 600;
`

export const TransactionFiatAmountBig = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text01};
  margin-bottom: 10px;
`

export const MessageBox = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: flex-start;
  justify-content: 'flex-start';
  flex-direction: column;
  border: 1px solid ${(p) => p.theme.color.divider01};
  box-sizing: border-box;
  border-radius: 4px;
  width: 255px;
  height: ${(p) => p.isApprove ? '120px' : '140px'};
  padding: ${(p) => p.isDetails ? '14px' : '4px 14px'};
  margin-bottom: 14px;
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;
`

export const TransactionTitle = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  font-weight: 600;
`

export const TransactionTypeText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  font-weight: 600;
`

export const ButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
`

export const FromToRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 8px;
`

export const ArrowIcon = styled(ArrowRightIcon)`
  width: auto;
  height: 16px;
  margin-right: 6px;
  margin-left: 6px;
  color: ${(p) => p.theme.color.text03};
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background-color: ${(p) => p.theme.color.divider01};
  border: 1px solid ${(p) => p.theme.color.divider01};
  margin-top: 6px;
  margin-bottom: 6px;
`

export const SectionRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
  height: inherit;
`

export const SectionColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  width: 100%;
  height: inherit;
  margin-bottom: 5px;
`

export const EditButton = styled(WalletButton)`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
`

export const TransactionText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.hasError ? p.theme.color.errorText : p.theme.color.text03};
  text-align: left;
`

export const FavIcon = styled.img`
  width: 48px;
  height: 48px;
  border-radius: 5px;
  background-color: ${(p) => p.theme.palette.grey200};
  margin-bottom: 7px;
`

export const QueueStepRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const QueueStepText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  color: ${(p) => p.theme.color.text02};
  font-weight: 600;
  margin-right: 9px;
`

export const QueueStepButton = styled(WalletButton) <Partial<StyleProps>>`
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  cursor: pointer;
  outline: none;
  border: none;
  margin: 0px;
  padding: 0px;
  margin-bottom: ${(p) => p.needsMargin ? '12px' : '0px'};
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.errorText};
  margin-bottom: 6px;
  text-align: center;
`

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const WarningIcon = styled(WarningBoxIcon)`
  width: 14px;
  height: 14px;
  margin-right: 6px;
`

export const ConfirmingButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: default;
  border-radius: 40px;
  padding: 8px 16px;
  outline: none;
  margin: 0px;
  background-color: ${(p) => p.theme.color.disabled};
  border: none;
`

export const ConfirmingButtonText = styled.span`
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.interactive07};
  flex: none;
  order: 1;
  flex-grow: 0;
  margin: 0px 8px;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 25px;
  width: 24px;
  opacity: .4;
`

export const GroupBox = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: 'flex-start';
  flex-direction: column;
  border: 1px solid ${(p) => p.theme.color.divider01};
  box-sizing: border-box;
  border-radius: 4px;
  width: 255px;
  min-height: 82px;
  padding: 4px 14px;
  overflow-y: scroll;
  overflow-x: hidden;
  position: relative;
  margin-top: 10px;
  background: ${p => p.theme.color.infoBackground};
`

export const GroupBoxColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  flex-direction: column;
`

export const GroupBoxTitle = styled.span`
  font-family: Poppins;
  font-size: 12px;
  font-weight: 600;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  word-break: break-all;
`

export const GroupBoxText = styled.div<{ dark: boolean }>`
  font-family: Poppins;
  font-size: 11px;
  font-weight: ${(p) => p.dark ? 600 : 400};
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.dark ? p.theme.color.text02 : p.theme.color.text03};
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
`

export const GroupEnumeration = styled.code`
  padding-right: 5px;
`

export const SmallLoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 16px;
  width: 16px;
  opacity: .4;
  padding-left: 5px;
`
