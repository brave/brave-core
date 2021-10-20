import styled from 'styled-components'
import { AlertCircleIcon } from 'brave-ui/components/icons'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding-top: 32px;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 6px;
`

export const Description = styled.span`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 450px;
  text-align: center;
  margin-bottom: 18px;
`

export const CopyButton = styled.button`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  color: ${(p) => p.theme.color.interactive07};
  margin-top: 16px;
`

export const TermsRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  margin-top: 26px;
  margin-bottom: 30px;
`

export const WarningBox = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  box-shadow: 0px 0px 8px rgba(151, 151, 151, 0.16);
  border-radius: 4px;
  padding: 12px 16px;
  margin-bottom: 34px;
  background-color: ${(p) => p.theme.color.background02};
`

export const WarningText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 600;
  color: ${(p) => p.theme.color.errorBorder};
`

export const DisclaimerColumn = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const DisclaimerText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  color: ${(p) => p.theme.color.text03};
`

export const AlertIcon = styled(AlertCircleIcon)`
  width: 30px;
  height: 30px;
  color: ${(p) => p.theme.color.errorBorder};
  margin-right: 16px;
`

export const RecoveryPhraseContainer = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  flex-direction: row;
  flex-wrap: wrap;
  width: 418px;
`

export const RecoveryBubble = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: ${(p) => p.theme.color.background01};
  padding: 5px 0px;
  border-radius: 4px;
  width: 100px;
  margin-bottom: 6px;
`

export const RecoveryBubbleText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`
