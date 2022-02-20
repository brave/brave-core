import * as React from 'react';
import styled from "styled-components";

interface Props {
    onDrop?: React.DragEventHandler<HTMLDivElement>;
    children: (isDraggedOver: boolean) => React.ReactNode; 
}

export function DropBoundary({ children, onDrop }: Props) {
    const [isDraggedOver, setIsDraggedOver] = React.useState(false);

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
            onDrop={(event) => {
                setIsDraggedOver(false)
                onDrop?.(event)
            }}
        >
            {children?.(isDraggedOver)}
        </Wrapper>
    )
}

const Wrapper = styled.div`
    display: flex;
    flex: 1;
    flex-direction: row;
    align-items: center;
    justify-content: center;
`;