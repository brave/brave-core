import styled from 'styled-components'

interface StyleProps {
  isSelected: boolean
  error: boolean
}

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
  max-width: 380px;
  height: 64px;
  text-align: center;
  margin-bottom: 4px;
`

export const SelectedPhraseContainer = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: flex-start;
  align-content: flex-start;
  justify-content: ${(p) => p.error ? 'center' : 'flex-start'};
  flex-direction: row;
  flex-wrap: wrap;
  width: 466px;
  min-height: 112px;
  margin-bottom: 40px;
  border: ${(p) => `1px solid ${p.theme.color.divider01}`};
  box-sizing: border-box;
  border-radius: 12px;
  padding: 8px 0px 0px 8px;
`

export const RecoveryPhraseContainer = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  flex-direction: row;
  flex-wrap: wrap;
  width: 418px;
  margin-bottom: 40px;
`

export const RecoveryBubble = styled.button<Partial<StyleProps>>`
  cursor: ${(p) => p.isSelected ? `default` : 'pointer'};
  outline: none;
  background: none;
  border: ${(p) => p.isSelected ? `1px solid ${p.theme.color.divider01}` : 'none'};
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: ${(p) => p.isSelected ? p.theme.color.background02 : p.theme.color.background01};
  padding: 5px 0px;
  border-radius: 4px;
  width: 100px;
  margin-bottom: 6px;
  height: 32px;
`

export const RecoveryBubbleText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 600;
  color: ${(p) => p.isSelected ? p.theme.color.background02 : p.theme.color.text01};
`

export const SelectedBubble = styled.button`
  cursor: pointer;
  outline: none;
  background: none;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: ${(p) => p.theme.color.background01};
  width: 106px;
  margin-right: 8px;
  margin-bottom: 8px;
  border: ${(p) => `1px solid ${p.theme.color.divider01}`};
  box-sizing: border-box;
  border-radius: 4px;
`

export const SelectedBubbleText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const ErrorContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  padding: 8px;
  background-color: ${(p) => p.theme.color.errorBackground};
  border: ${(p) => `1px solid ${p.theme.color.errorBorder}`};
  border-radius: 4px;
  position: absolute;
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  color: ${(p) => p.theme.color.errorText};
`
