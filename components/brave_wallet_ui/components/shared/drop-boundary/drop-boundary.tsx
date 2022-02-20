import * as React from 'react';
import { PropsWithChildren } from "react";
import styled from "styled-components";

interface Props extends PropsWithChildren<{}> {
    onDrop?: React.DragEventHandler<HTMLDivElement>;
    // onDropAfter?: React.DragEventHandler<HTMLDivElement>;
}

export function DropBoundary({ children, onDrop }: Props) {
    const [_isDraggedOver, setIsDraggedOver] = React.useState(false);

    return (
        <Wrapper
            onDragOver={(event) => {
                event.preventDefault();
                setIsDraggedOver(true)
            }}
            onDragLeave={(event) => {
                event.preventDefault();
                setIsDraggedOver(false)
            }}
            onDrop={onDrop}
        >
            {/* {isDraggedOver ? <ShadowSlot /> : null} */}
            {children}
        </Wrapper>
    )
}

const Wrapper = styled.div<{
    isDraggedOver?: boolean;
}>`
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: center;    
`;

// const ShadowSlot = styled.div<{
//     isDraggedOver?: boolean;
// }>`
//     width: 110px;
//     height: 100%;
//     background: ${(p) => p.theme.color.divider01}; 
//     content: " ";   
// `;