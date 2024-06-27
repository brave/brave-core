/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
//@ts-nocheck
import React, { useState, useRef, useEffect, useCallback } from 'react';
import { pdfjs, Document, Page } from 'react-pdf';
import 'react-pdf/dist/esm/Page/AnnotationLayer.css';

pdfjs.GlobalWorkerOptions.workerSrc = new URL(
  './pdfjs-dist-worker.js',
  import.meta.url
).toString();

interface Props {
  onEnable?: () => void;
}

export function PdfRenderer(props: Props) {
  const [pdfFile, setPdfFile] = useState(null);
  const [numPages, setNumPages] = useState(null);
  const [pageNumber, setPageNumber] = useState(1);
  const [isSelecting, setIsSelecting] = useState(false);
  const [isSelectionEnabled, setIsSelectionEnabled] = useState(false);
  const [isSigned, setIsSigned] = useState(false);
  const [selectionCoords, setSelectionCoords] = useState({ startX: 0, startY: 0, endX: 0, endY: 0 });

  const overlayCanvasRefs = useRef([]);
  const pdfCanvasRefs = useRef([]);
  const pdfContainerRef = useRef(null);
  const pageRefs = useRef([]);
  const fixedText = "Signed by user";

  const onDocumentLoadSuccess = ({ numPages }) => {
    setNumPages(numPages);
    setPageNumber(1);
  };

  const onPageLoadSuccess = useCallback((pageIndex) => {
    const pageCanvas = pdfCanvasRefs.current[pageIndex];
    const overlayCanvas = overlayCanvasRefs.current[pageIndex];
    if (pageCanvas && overlayCanvas) {
      overlayCanvas.width = pageCanvas.width;
      overlayCanvas.height = pageCanvas.height;
    }
    // Set the max width of the pdfContainerRef to the width of the page
    if (pageCanvas && pdfContainerRef.current) {
      pdfContainerRef.current.style.maxWidth = `${pageCanvas.width}px`;
    }
  }, []);

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

  const handleMouseDown = (event, pageIndex) => {
    if (!isSelectionEnabled) return;
    setIsSelecting(true);
    const pos = getMousePos(overlayCanvasRefs.current[pageIndex], event);
    setSelectionCoords({ startX: pos.x, startY: pos.y, endX: pos.x, endY: pos.y });
  };

  const handleMouseMove = (event, pageIndex) => {
    if (!isSelecting || !isSelectionEnabled) return;
    const pos = getMousePos(overlayCanvasRefs.current[pageIndex], event);
    setSelectionCoords({ ...selectionCoords, endX: pos.x, endY: pos.y });
    drawSelection(pos.x, pos.y, pageIndex);
  };

  const handleMouseUp = (pageIndex) => {
    if (!isSelectionEnabled) return;
    setIsSelecting(false);
    showEmbedSignConfirmation(pageIndex);
  };

  const drawSelection = (endX, endY, pageIndex) => {
    const { startX, startY } = selectionCoords;
    const overlayCtx = overlayCanvasRefs.current[pageIndex].getContext('2d');
    clearOverlay(pageIndex);
    overlayCtx.strokeStyle = 'blue';
    overlayCtx.lineWidth = 2;
    overlayCtx.strokeRect(startX, startY, endX - startX, endY - startY);
  };

  const clearOverlay = (pageIndex) => {
    const overlayCtx = overlayCanvasRefs.current[pageIndex].getContext('2d');
    overlayCtx.clearRect(0, 0, overlayCanvasRefs.current[pageIndex].width, overlayCanvasRefs.current[pageIndex].height);
  };

  const showEmbedSignConfirmation = (pageIndex) => {
    const confirmation = window.confirm("Do you want to embed text in the selected area?");
    if (confirmation) {
      embedSignature(pageIndex);
    } else {
      clearSelection(pageIndex);
    }
  };

  const embedSignature = (pageIndex) => {
    const { startX, startY, endX, endY } = selectionCoords;
    const overlayCtx = overlayCanvasRefs.current[pageIndex].getContext('2d');
    overlayCtx.fillStyle = 'rgba(255, 255, 0, 0.5)';
    overlayCtx.fillRect(startX, startY, endX - startX, endY - startY);

    overlayCtx.font = '14px Arial';
    overlayCtx.fillStyle = 'black';
    overlayCtx.textAlign = 'left';
    overlayCtx.textBaseline = 'top';
    overlayCtx.fillText(fixedText, startX + 5, startY + 5);

    setIsSelectionEnabled(false);
    setIsSigned(true);
    console.log(`Page number: ${pageIndex + 1}, Selected area coordinates: Start(${startX}, ${startY}) - End(${endX}, ${endY})`);
    document.getElementById('signButton').disabled = true;
  };

  const clearSelection = (pageIndex) => {
    setSelectionCoords({ startX: 0, startY: 0, endX: 0, endY: 0 });
    clearOverlay(pageIndex);
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
      scrollToPage(pageNumber - 1);
    }
  };

  const handleNextPage = () => {
    if (pageNumber < numPages) {
      setPageNumber(pageNumber + 1);
      scrollToPage(pageNumber + 1);
    }
  };

  const handlePageInputChange = (event) => {
    const newPageNumber = parseInt(event.target.value, 10);
    if (newPageNumber >= 1 && newPageNumber <= numPages) {
      setPageNumber(newPageNumber);
      scrollToPage(newPageNumber);
    }
  };

  const scrollToPage = (pageNum) => {
    const pageElement = pageRefs.current[pageNum - 1];
    if (pageElement) {
      pageElement.scrollIntoView({ behavior: 'smooth' });
    }
  };

  useEffect(() => {
    const observer = new IntersectionObserver(
      (entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            const pageIndex = pageRefs.current.indexOf(entry.target) + 1;
            setPageNumber(pageIndex);
          }
        });
      },
      { root: pdfContainerRef.current, rootMargin: '0px', threshold: 0.7 }
    );

    pageRefs.current.forEach((page) => {
      if (page) observer.observe(page);
    });

    return () => {
      pageRefs.current.forEach((page) => {
        if (page) observer.unobserve(page);
      });
    };
  }, [numPages]);

  return (
    <>
      <div className="App" style={{ background: "gray", padding: '20px' }}>
        <div id="controls" style={{ 
            display: 'flex', 
            justifyContent: "center", 
            marginBottom: '20px',
            position: 'fixed',
            top: 0,
            left: 0,
            right: 0,
            background: 'white',
            zIndex: 1000,
            padding: '10px 0',
            boxShadow: '0px 2px 10px rgba(0, 0, 0, 0.1)'
          }}>
          <input 
            type="file" 
            id="pdfInput" 
            accept="application/pdf" 
            onChange={handleFileInput} 
            style={{ padding: '10px', marginRight: '10px' }} 
          />
          <button 
            id="signButton" 
            onClick={handleSignButtonClick} 
            disabled={isSigned} 
            style={{ padding: '10px', marginRight: '10px', borderRadius: '5px' }}
          >
            Sign
          </button>
          <button 
            onClick={handlePreviousPage} 
            disabled={pageNumber === 1} 
            style={{ padding: '10px', marginRight: '10px', borderRadius: '10px' }}
          >
            Previous Page
          </button>
          <button 
            onClick={handleNextPage} 
            disabled={pageNumber === numPages} 
            style={{ padding: '10px', marginRight: '10px', borderRadius: '10px' }}
          >
            Next Page
          </button>
          <input
            type="number"
            value={pageNumber}
            onChange={handlePageInputChange}
            min={1}
            max={numPages}
            style={{ width: '60px', padding: '10px', textAlign: 'center' }}
          />
        </div>
        <div
          id="pdfContainer"
          ref={pdfContainerRef}
          style={{
            overflowX: 'hidden',
            overflowY: 'auto',
            height: '100vh',
            backgroundColor: 'white',
            margin: '55px auto',
            width: '65vw',
            display: 'flex',
            justifyContent: 'center',
            padding: '20px',
            boxShadow: '0px 0px 10px rgba(0, 0, 0, 0.1)'
          }}
        >
          <Document
            file={pdfFile}
            onLoadSuccess={onDocumentLoadSuccess}
            loading={<div>Loading PDF...</div>}
          >
            {numPages &&
              Array.from({ length: numPages }, (_, index) => (
                <div key={`page_${index + 1}`} style={{ position: 'relative', marginBottom: '20px' }} ref={(el) => (pageRefs.current[index] = el)}>
                  <Page
                    pageNumber={index + 1}
                    renderTextLayer={false}
                    renderMode="canvas"
                    onLoadSuccess={() => onPageLoadSuccess(index)}
                    canvasRef={(el) => (pdfCanvasRefs.current[index] = el)}
                    loading={<div>Loading page...</div>}
                  />
                  <canvas
                    id={`overlayCanvas_${index}`}
                    ref={(el) => (overlayCanvasRefs.current[index] = el)}
                    style={{
                      position: 'absolute',
                      top: 0,
                      left: 0,
                      pointerEvents: isSelectionEnabled ? 'auto' : 'none',
                    }}
                    onMouseDown={(e) => handleMouseDown(e, index)}
                    onMouseMove={(e) => handleMouseMove(e, index)}
                    onMouseUp={() => handleMouseUp(index)}
                  />
                </div>
              ))}
          </Document>
        </div>
      </div>
    </>
  );
}
