/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
//@ts-nocheck
import * as React from 'react'
import * as urls from '../../lib/rewards_urls'
import { useState, useRef, useEffect } from 'react';
import { pdfjs, Document, Page } from 'react-pdf';
import 'react-pdf/dist/esm/Page/AnnotationLayer.css';

pdfjs.GlobalWorkerOptions.workerSrc = new URL(
  './pdfjs-dist-worker.js',
  import.meta.url
).toString();

interface Props {
  onEnable?: () => void
}

export function PdfRenderer (props: Props) {

  // const { getString } = React.useContext(LocaleContext)
  const [pdfFile, setPdfFile] = useState(null);
  const [numPages, setNumPages] = useState(null);
  const [pageNumber, setPageNumber] = useState(1);
  const [isSelecting, setIsSelecting] = useState(false);
  const [isSelectionEnabled, setIsSelectionEnabled] = useState(false);
  const [isSigned, setIsSigned] = useState(false);
  const [startX, setStartX] = useState(0);
  const [startY, setStartY] = useState(0);
  const [endX, setEndX] = useState(0);
  const [endY, setEndY] = useState(0);

  const overlayCanvasRef = useRef(null);
  const pdfCanvasRef = useRef(null);
  const fixedText = "Signed by user";

  const onDocumentLoadSuccess = ({ numPages }) => {
    setNumPages(numPages);
    setPageNumber(1);
  };

  const onPageLoadSuccess = () => {
    const pageCanvas = pdfCanvasRef.current;
    const overlayCanvas = overlayCanvasRef.current;
    if (pageCanvas && overlayCanvas) {
      overlayCanvas.width = pageCanvas.width;
      overlayCanvas.height = pageCanvas.height;
    }
  };

  const handleFileInput = (event) => {
    const file = event.target.files[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = () => {
        setPdfFile(reader.result);
      };
      reader.readAsArrayBuffer(file);
    }
  };

  const getMousePos = (canvas, event) => {
    const rect = canvas.getBoundingClientRect();
    return {
      x: event.clientX - rect.left,
      y: event.clientY - rect.top,
    };
  };

  const handleMouseDown = (event) => {
    if (!isSelectionEnabled) return;
    setIsSelecting(true);
    const pos = getMousePos(overlayCanvasRef.current, event);
    setStartX(pos.x);
    setStartY(pos.y);
    setEndX(pos.x);
    setEndY(pos.y);
  };

  const handleMouseMove = (event) => {
    if (!isSelecting || !isSelectionEnabled) return;
    const pos = getMousePos(overlayCanvasRef.current, event);
    setEndX(pos.x);
    setEndY(pos.y);
    drawSelection();
  };

  const handleMouseUp = () => {
    if (!isSelectionEnabled) return;
    setIsSelecting(false);
    showEmbedSignConfirmation();
  };

  const drawSelection = () => {
    const overlayCtx = overlayCanvasRef.current.getContext('2d');
    clearOverlay();
    overlayCtx.strokeStyle = 'blue';
    overlayCtx.lineWidth = 2;
    overlayCtx.strokeRect(startX, startY, endX - startX, endY - startY);
  };

  const clearOverlay = () => {
    const overlayCtx = overlayCanvasRef.current.getContext('2d');
    overlayCtx.clearRect(0, 0, overlayCanvasRef.current.width, overlayCanvasRef.current.height);
  };

  const showEmbedSignConfirmation = () => {
    const confirmation = window.confirm("Do you want to embed text in the selected area?");
    if (confirmation) {
      embedSignature();
    } else {
      clearSelection();
    }
  };

  const embedSignature = () => {
    const overlayCtx = overlayCanvasRef.current.getContext('2d');
    overlayCtx.fillStyle = 'rgba(255, 255, 0, 0.5)';
    overlayCtx.fillRect(startX, startY, endX - startX, endY - startY);

    overlayCtx.font = '14px Arial';
    overlayCtx.fillStyle = 'black';
    overlayCtx.textAlign = 'left';
    overlayCtx.textBaseline = 'top';
    overlayCtx.fillText(fixedText, startX + 5, startY + 5);

    setIsSelectionEnabled(false);
    setIsSigned(true);
    console.log(`Selected area coordinates: Start(${startX}, ${startY}) - End(${endX}, ${endY})`);
    document.getElementById('signButton').disabled = true;
  };

  const clearSelection = () => {
    setStartX(0);
    setStartY(0);
    setEndX(0);
    setEndY(0);
    clearOverlay();
  };

  const handleSignButtonClick = () => {
    if (!isSigned) {
      setIsSelectionEnabled(true);
      console.log("Selection tool enabled. Click and drag on the PDF to select an area.");
    }
  };

  const handlePreviousPage = () => {
    if (pageNumber > 1) {
      setPageNumber(pageNumber - 1);
    }
  };
  
  const handleNextPage = () => {
    if (pageNumber < numPages) {
      setPageNumber(pageNumber + 1);
    }
  };
  
  const handlePageInputChange = (event) => {
    const newPageNumber = parseInt(event.target.value, 10);
    if (newPageNumber >= 1 && newPageNumber <= numPages) {
      setPageNumber(newPageNumber);
    }
  };

  return (
    <>
      <div className="App">
      <div id="controls">
        <input type="file" id="pdfInput" accept="application/pdf" onChange={handleFileInput} />
        <button id="signButton" onClick={handleSignButtonClick} disabled={isSigned}>Sign</button>
        <button onClick={handlePreviousPage} disabled={pageNumber === 1}>Previous Page</button>
        <button onClick={handleNextPage} disabled={pageNumber === numPages}>Next Page</button>
        <input
          type="number"
          value={pageNumber}
          onChange={handlePageInputChange}
          min={1}
          max={numPages}
          style={{ width: '60px' }}
        />
      </div>
      <div id="canvasContainer" style={{ position: 'relative' }}>
        <Document
          file={pdfFile}
          onLoadSuccess={onDocumentLoadSuccess}
          loading={<div>Loading PDF...</div>}
        >
          <Page
            pageNumber={pageNumber}
            renderTextLayer={false}
            width={600}
            renderMode="canvas"
            onLoadSuccess={onPageLoadSuccess}
            canvasRef={pdfCanvasRef}
            loading={<div>Loading page...</div>}
          />
        </Document>
        <canvas
          id="overlayCanvas"
          ref={overlayCanvasRef}
          style={{
            position: 'absolute',
            top: 0,
            left: 0,
            pointerEvents: isSelectionEnabled ? 'auto' : 'none',
          }}
          onMouseDown={handleMouseDown}
          onMouseMove={handleMouseMove}
          onMouseUp={handleMouseUp}
        />
      </div>
    </div>
    </>
  )
}
