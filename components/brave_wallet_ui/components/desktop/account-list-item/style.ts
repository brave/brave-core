import styled from 'styled-components'
import TrashIcon from '../../../assets/svg-icons/trash-icon.svg'
import FlashdriveIcon from '../../../assets/svg-icons/flashdrive-icon.svg'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  margin-bottom: 15px;
`

export const NameAndIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
`

export const AccountAndAddress = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: center;
  flex-direction: column;
`

export const AccountNameRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-direction: row;
`

export const AccountName = styled.button`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const AccountAddress = styled.button`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const RightSide = styled.div`
  display: flex;
  align-items: flex-end;
  justify-content: center;
  flex-direction: row;
`

export const AccountCircle = styled.div<StyleProps>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-right: 12px;
`

export const DeleteButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
`

export const DeleteIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${TrashIcon});
  mask-image: url(${TrashIcon});
`

export const HardwareIcon = styled.div`
  width: 13px;
  height: 13px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${FlashdriveIcon});
  mask-image: url(${FlashdriveIcon});
`
