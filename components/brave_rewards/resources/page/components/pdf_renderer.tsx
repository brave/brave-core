/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
// @ts-nocheck
import React, { useState, useCallback, useEffect, useRef } from 'react';
import { pdfjs, Document, Page } from 'react-pdf';
import 'react-pdf/dist/esm/Page/AnnotationLayer.css';
import styles from './ping-sign-pdf.module.css';

pdfjs.GlobalWorkerOptions.workerSrc = new URL(
  './pdfjs-dist-worker.js',
  import.meta.url,
).toString();

let ws;
const MAX_RECONNECT_ATTEMPTS = 5;
let reconnectAttempts = 0;

const tooltip = ({ content }) => {
  return (
    <div className={styles.tooltipContainer}>
      {content};
    </div>
  )
}

const SignaturePopup = ({ onClose, onConfirm }) => {
  const [selectedSignature, setSelectedSignature] = useState(null);

  const signatures = [
    { id: '1', name: 'Presley Abernathy', issueDate: '05/07/2024 15:30' },
    { id: '2', name: 'Harrison Wilderman', issueDate: '05/07/2024 15:30' },
    { id: '3', name: 'Rudolf Wolf', issueDate: '05/07/2024 15:30' },
  ];

  const handleConfirm = () => {
    if (selectedSignature) {
      onConfirm(selectedSignature);
    }
  };

  return (
    <div className={styles.popupOverlay}>
      <div className={`${styles.popupContent} ${selectedSignature ? styles.selected : ''}`}>
        <h2 className={styles.h2}>Choose a digital ID to sign with:</h2>
        <button className={styles.closeButton} onClick={onClose}>×</button>

        {selectedSignature && (
          <div className={styles.selectedSignature}>
            <h3>{signatures.find(sig => sig.id === selectedSignature).name}</h3>
            <p>Project manager, Apple</p>
            <p>presleyabernathy@gmail.com</p>
            <p>05/07/2024, IST 21:35</p>
            <p className={styles.encKey}>Enc. Key: 87478632758654</p>
            <div className={styles.browseImage}>Browse for Image</div>
          </div>
        )}

        <div className={styles.signatureList}>
          {signatures.map(sig => (
            <label key={sig.id} className={styles.signatureOption}>
              <input
                type="radio"
                name="signature"
                value={sig.id}
                checked={selectedSignature === sig.id}
                onChange={() => setSelectedSignature(sig.id)}
              />
              <div className={styles.signatureName}>
                <span className={styles.issueName}>{sig.name}</span>
                <span className={styles.issueDate}>Issued: {sig.issueDate}</span>
              </div>
            </label>
          ))}
        </div>
        <div className={styles.buttons}>
          <button className={styles.addButton}>+ Add</button>
          <button
            className={styles.confirmButton}
            onClick={handleConfirm}
            disabled={!selectedSignature}
          >
            Confirm signature
          </button>
        </div>
      </div>
    </div>
  );
};

const SignatureMethodPopup = ({ onClose, onSelectMethod }) => (
  <div className={styles.popupOverlay}>
    <div className={styles.popupContent}>
      <h2 className={styles.h2}>Choose Your Digital Signature Method</h2>
      <button className={styles.closeButton} onClick={onClose}>×</button>
      <div className={styles.methodOptions}>
        <div className={styles.button} onClick={() => onSelectMethod('digitalID')}>
          <div className={styles.buttonTitle}>Sign with digital ID (Recommended)</div>
          <div className={styles.buttonDesc}>Sign documents quickly using your pre-uploaded signature data for a seamless and efficient signing process</div>
        </div>
        <div className={styles.button} onClick={() => onSelectMethod('imageUpload')}>
          <div className={styles.buttonTitle}>Upload Image Signature</div>
          <div className={styles.buttonDesc}>Select and upload an image of your signature from your device to sign documents easily and securely.</div>
        </div>
      </div>
    </div>
  </div>
);

const SuccessPopup = ({ message, onSave, onContinue, isVerification }) => (
  <div className={styles.successPopup}>
    {isVerification ? (
      <h2 className={`${styles.successTitle} ${styles.successVerificationTitle}`}>Verification Successful!</h2>
    ) : (
      <h2 className={styles.successTitle}>Signature complete!</h2>
    )}
    <p className={styles.successMessage}>{message}</p>
    {!isVerification ? (
      <p className={styles.successName}>Placeholder</p>
    ) : (
      <p></p>
    )}
    <div className={styles.successButtons}>
      <button className={`${styles.confirmButton} ${styles.btn}`} onClick={onSave}>Save as</button>
      <button className={`${styles.confirmButton} ${styles.continue}`} onClick={onContinue}>Continue</button>
    </div>
  </div>
);

const AnimatedStatus = ({ message, type }) => (
  <div className={`${styles.animatedStatus} ${styles[type]} ${styles.visible}`}>
    <div className={styles.statusContent}>
      Status: <span className={styles.tex}>{message}</span>
    </div>
  </div>
);

export function PdfRenderer() {
  const [pdfFile, setPdfFile] = useState(null);
  const [pdfFileName, setPdfFileName] = useState('');
  const [numPages, setNumPages] = useState(null);
  const [pdfBuff, setPdfBuff] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [isSelecting, setIsSelecting] = useState(false);
  const [isSelectionEnabled, setIsSelectionEnabled] = useState(false);
  const [selectionCoords, setSelectionCoords] = useState({ startX: 1, startY: 1, endX: 1, endY: 1 });
  const [currentPageIndex, setCurrentPageIndex] = useState(null);
  const [hsmPath, setHsmPath] = useState('');
  const [selectedSignature, setSelectedSignature] = useState(null);
  const [pageNumber, setPageNumber] = useState(1);
  const [isEditingPageNumber, setIsEditingPageNumber] = useState(false);
  const [tempPageNumber, setTempPageNumber] = useState('');
  const [isDragging, setIsDragging] = useState(false);
  const [showSignaturePopup, setShowSignaturePopup] = useState(false);
  const [showSuccessPopup, setShowSuccessPopup] = useState(false);
  const [successMessage, setSuccessMessage] = useState('');
  const [showSignatureMethodPopup, setShowSignatureMethodPopup] = useState(false);
  const [selectedSignatureImage, setSelectedSignatureImage] = useState(null);
  const [isVerification, setIsVerification] = useState(false);
  const [isVerified, setIsVerified] = useState(false);
  const [isVerificationFailed, setIsVerificationFailed] = useState(false);
  const [statusMessage, setStatusMessage] = useState('');
  const [isStatusVisible, setIsStatusVisible] = useState(false);
  const [statusType, setStatusType] = useState('checking');
  const overlayCanvasRefs = useRef([]);
  const pdfCanvasRefs = useRef([]);
  const pdfContainerRef = useRef(null);
  const pageRefs = useRef([]);
  const fileInputRef = useRef(null);

  const connectWebSocket = () => {
    ws = new WebSocket('ws://localhost:5000');

    ws.onopen = () => {
      console.log('WebSocket connected');
      reconnectAttempts = 0;
    };

    ws.onclose = () => {
      console.log('WebSocket disconnected');
      if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        setTimeout(connectWebSocket, 3000);
        reconnectAttempts++;
      } else {
        alert('Unable to connect to the server. Please check your connection and refresh the page.');
      }
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      alert('Connection error. Please try again later.');
    };

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.action === 'signed') {
        const signedPdfBuffer = new Uint8Array(Buffer.from(data.data, 'base64'));
        setPdfFile(new Blob([signedPdfBuffer], { type: 'application/pdf' }));
        setPdfBuff(signedPdfBuffer);
        setStatusMessage('Signature Complete');
        setStatusType('success');
        setIsStatusVisible(true);
        setTimeout(() => {
          setIsStatusVisible(false);
        }, 3000);
        setIsSelectionEnabled(false);
        setSuccessMessage(`Your document has been signed`);
        setShowSuccessPopup(true);
        setIsLoading(false);
        clearAllSelections();
      } else if (data.action === 'verified') {
        setIsLoading(false);
        if (data.verified) {
          setStatusMessage('Verification Successful');
          setStatusType('success');
          setIsVerified(true);
          setIsVerificationFailed(false);
        } else {
          setStatusMessage('Verification Failed');
          setStatusType('error');
          setIsVerified(false);
          setIsVerificationFailed(true);
        }
        setIsStatusVisible(true);
        setTimeout(() => {
          setIsStatusVisible(false);
          if (data.verified) {
            setSuccessMessage('Document verification successful!');
            setIsVerification(true);
            setShowSuccessPopup(false);
          }
        }, 3000);
      } else if (data.action === 'error') {
        console.error('Server error:', data.message);
        setStatusMessage('Error');
        setStatusType('error');
        setIsStatusVisible(true);
        setTimeout(() => {
          setIsStatusVisible(false);
          alert(`Error: ${data.message}`);
        }, 3000);
        setIsLoading(false);
      }
    };
  };

  useEffect(() => {
    connectWebSocket();

    return () => {
      if (ws) {
        ws.close();
      }
    };
  }, []);

  const sendWebSocketMessage = (message) => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(message));
    } else {
      alert('Not connected to the server. Please wait for the connection to be established.');
      alert('Connection to server lost. Attempting to reconnect...');
      connectWebSocket();
    }
  };

  const handleFileInput = useCallback(
    async (event) => {
      const file = event.target.files ? event.target.files[0] : event.dataTransfer.files[0];
      if (file && file.type === 'application/pdf') {
        try {
          const arrayBuffer = await file.arrayBuffer();
          setPdfBuff(arrayBuffer);
          setPdfFile(new Blob([arrayBuffer], { type: 'application/pdf' }));
          setPdfFileName(file.name);
        } catch (error) {
          console.error('Error reading PDF file:', error);
          alert('Error reading PDF file. Please try again.');
        }
      } else {
        alert('Please upload a valid PDF file.');
      }
    },
    []
  );

  const handleDrop = useCallback(
    (event) => {
      event.preventDefault();
      setIsDragging(false);
      handleFileInput(event);
    },
    [handleFileInput]
  );

  const handleLogoClick = () => {
    if (!pdfFile) {
      fileInputRef.current.click();
    }
  };

  const promptHsmPath = () => {
    const path = prompt('Enter HSM path:');
    if (path) setHsmPath(path);
    return path;
  };

  const getMousePos = (canvas, event) => {
    const rect = canvas.getBoundingClientRect();
    return {
      x: event.clientX - rect.left,
      y: event.clientY - rect.top,
    };
  };

  const isSelectionValid = () => {
    const { startX, startY, endX, endY } = selectionCoords;
    const width = Math.abs(endX - startX);
    const height = Math.abs(endY - startY);
    const minSize = 50; // Minimum size in pixels
    return width >= minSize && height >= minSize;
  };

  const handleMouseDown = (event, pageIndex) => {
    if (!isSelectionEnabled) return;
    setIsSelecting(true);
    const pos = getMousePos(overlayCanvasRefs.current[pageIndex], event);
    setSelectionCoords({ startX: pos.x, startY: pos.y, endX: pos.x, endY: pos.y });
    setCurrentPageIndex(pageIndex);
  };

  const handleMouseMove = (event, pageIndex) => {
    if (!isSelecting || !isSelectionEnabled) return;
    const pos = getMousePos(overlayCanvasRefs.current[pageIndex], event);
    setSelectionCoords(prev => ({ ...prev, endX: pos.x, endY: pos.y }));
    drawSelection(pos.x, pos.y, pageIndex);
  };

  const handleMouseUp = (pageIndex) => {
    if (!isSelectionEnabled) return;
    setIsSelecting(false);
    if (isSelectionValid()) {
      showEmbedSignConfirmation(pageIndex);
    } else {
      alert('Selected area is too small. Please select a larger area.');
      clearSelection(pageIndex);
    }
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

  const clearAllSelections = () => {
    overlayCanvasRefs.current.forEach((canvas, index) => {
      if (canvas) {
        clearOverlay(index);
      }
    });
  };

  const showEmbedSignConfirmation = (pageIndex) => {
    const confirmation = window.confirm("Do you want to embed the signature in the selected area?");
    if (confirmation) {
      setCurrentPageIndex(pageIndex);
      sendSignRequest();
    } else {
      clearSelection(pageIndex);
    }
  };

  const sendSignRequest = () => {
    setIsLoading(true);
    setStatusMessage('Signing ...');
    setStatusType('checking');
    setIsStatusVisible(true);
    sendWebSocketMessage({
      action: 'sign',
      data: {
        pdfBuffer: Array.from(new Uint8Array(pdfBuff)),
        pageIndex: currentPageIndex,
        selectionCoords: selectionCoords,
        signatureType: selectedSignatureImage ? 'image' : 'digitalID',
        hsmPath: selectedSignatureImage ? null : hsmPath,
        signatureId: selectedSignatureImage ? null : selectedSignature,
        signatureImage: selectedSignatureImage,
      },
    });
  };

  const handleSignButtonClick = useCallback(() => {
    if (!pdfBuff) {
      alert('Please upload a PDF first');
      return;
    }
    setStatusMessage('Preparing to sign ...');
    setStatusType('checking');
    setIsStatusVisible(true);
    setShowSignatureMethodPopup(true);
  }, [pdfBuff]);

  const handleCloseSignaturePopup = () => {
    setShowSignaturePopup(false);
    setIsStatusVisible(false);
  };

  const handleConfirmation = (signature) => {
    setSelectedSignature(signature);
    setShowSignaturePopup(false);
    setIsSelectionEnabled(true);
  };

  const handleSelectSignatureMethod = (method) => {
    setShowSignatureMethodPopup(false);
    if (method === 'digitalID') {
      setShowSignaturePopup(true);
    } else if (method === 'imageUpload') {
      handleImageUpload();
    }
  };

  const handleImageUpload = () => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = 'image/*';
    input.onchange = (e) => {
      const file = e.target.files[0];
      if (file) {
        const reader = new FileReader();
        reader.onload = (event) => {
          const imageData = event.target.result;
          setIsSelectionEnabled(true);
          setSelectedSignatureImage(imageData);
        };
        reader.readAsDataURL(file);
      }
    };
    input.click();
  };

  const handleVerifyButtonClick = useCallback(() => {
    if (!pdfBuff) {
      alert('Please upload a PDF first');
      return;
    }
    setStatusMessage('Checking ...');
    setStatusType('checking');
    setIsStatusVisible(true);
    sendWebSocketMessage({
      action: 'verify',
      data: {
        Buff: Array.from(new Uint8Array(pdfBuff)),
      },
    });
  }, [pdfBuff]);

  const handleDownloadButtonClick = () => {
    if (pdfFile) {
      const link = document.createElement('a');
      link.href = URL.createObjectURL(pdfFile);
      link.download = 'signed_document.pdf';
      link.click();
    }
  };

  const onDocumentLoadSuccess = useCallback(({ numPages }) => {
    setNumPages(numPages);
    setIsVerified(false);
    setPageNumber(1);
  }, []);

  const onPageLoadSuccess = useCallback((pageIndex) => {
    const pageCanvas = pdfCanvasRefs.current[pageIndex];
    const overlayCanvas = overlayCanvasRefs.current[pageIndex];
    if (pageCanvas && overlayCanvas) {
      overlayCanvas.width = pageCanvas.width;
      overlayCanvas.height = pageCanvas.height;
    }
    if (pageCanvas && pdfContainerRef.current) {
      pdfContainerRef.current.style.maxWidth = `${pageCanvas.width}px`;
    }
  }, []);

  const scrollToPage = (pageNum) => {
    const pageElement = pageRefs.current[pageNum - 1];
    if (pageElement) {
      pageElement.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
  };

  const handlePreviousPage = useCallback(() => {
    setPageNumber((prevPageNumber) => {
      const newPageNumber = Math.max(prevPageNumber - 1, 1);
      scrollToPage(newPageNumber);
      return newPageNumber;
    });
  }, []);

  const handleNextPage = useCallback(() => {
    setPageNumber((prevPageNumber) => {
      const newPageNumber = numPages ? Math.min(prevPageNumber + 1, numPages) : prevPageNumber;
      scrollToPage(newPageNumber);
      return newPageNumber;
    });
  }, [numPages]);

  const handlePageNumberClick = () => {
    setIsEditingPageNumber(true);
    setTempPageNumber(pageNumber.toString());
  };

  const handlePageNumberChange = (e) => {
    setTempPageNumber(e.target.value);
  };

  const handlePageNumberSubmit = (e) => {
    e.preventDefault();
    const newPageNumber = parseInt(tempPageNumber, 10);
    if (!isNaN(newPageNumber) && newPageNumber >= 1 && newPageNumber <= numPages) {
      setPageNumber(newPageNumber);
      scrollToPage(newPageNumber);
    }
    setIsEditingPageNumber(false);
  };

  const handleSaveAs = () => {
    handleDownloadButtonClick();
    setShowSuccessPopup(false);
  };

  const handleContinue = () => {
    setShowSuccessPopup(false);
  };

  useEffect(() => {
    const handleKeyDown = (event) => {
      if (event.key === 'ArrowLeft') {
        handlePreviousPage();
      } else if (event.key === 'ArrowRight') {
        handleNextPage();
      }
    };

    window.addEventListener('keydown', handleKeyDown);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [handlePreviousPage, handleNextPage]);

  const DropZone = ({ onFileInput, isDragging }) => {
    const fileInputRef = useRef(null);

    const handleLogoClick = () => {
      fileInputRef.current.click();
    };

    return (
      <div className={styles.dropZoneContainer}>
        <div
          className={`${styles.dropZone} ${isDragging ? styles.dragging : ''}`}
          onDragOver={(e) => e.preventDefault()}
          onDragEnter={() => setIsDragging(true)}
          onDragLeave={() => setIsDragging(false)}
          onDrop={handleDrop}
        >
          <img
            src="../assets/pdfLogo.png"
            alt="PDF Logo"
            className={styles.pdfLogo}
            onClick={handleLogoClick}
          />
          <p className={styles.pdfText}>Choose a PDF file to add your digital signature</p>
          <label htmlFor="fileInput" className={styles.addFileButton}>
            + Add file
          </label>
          <input
            id="fileInput"
            ref={fileInputRef}
            type="file"
            accept=".pdf"
            onChange={onFileInput}
            style={{ display: 'none' }}
          />
          <p className={styles.legalText}>
            By clicking on add file, you agree to Ping's <br />
            <a href="#" style={{ color: '#2BB563', textDecoration: 'none' }}> Privacy policy </a>
            &
            <a href="#" style={{ color: '#2BB563', textDecoration: 'none' }}> Terms of use</a>
          </p>
        </div>
      </div>
    );
  };

  return (
    <div className={styles.app}>
      <header className={styles.header}>
        <div className={styles.navBar}>
          <img
            src="../assets/pdfMain.png"
            alt="PDF Logo"
            className={styles.logo}
            onClick={handleLogoClick}
          />
          <div className={styles.pdfFileName}>{pdfFileName}</div>
          {pdfFile && !isSelectionEnabled ? (
            <div className={styles.headerControls}>
              {isStatusVisible ? (
                <AnimatedStatus message={statusMessage} type={statusType} />
              ) : (
                <div className={`${styles.fadeAway} ${isStatusVisible ? styles.fadeAnimation : ""}`}>
                  <button className={styles.headerButton} onClick={handleSignButtonClick}>Add signature</button>
                  <div className={styles.headerControlsBar}></div>
                  <button className={`${styles.headerButton} ${isVerified ? styles.verified : ""} ${isVerificationFailed ? styles.notVerified : ""}`} onClick={handleVerifyButtonClick}>Verify document</button>
                </div>
              )}
              <div className={styles.headerControlsBar}></div>
              <div className={styles.pageChangingControls}>
                <div className={styles.previousPage} onClick={handlePreviousPage}>&lt;</div>
                <div className={styles.pageNumber}>
                  {isEditingPageNumber ? (
                    <form onSubmit={handlePageNumberSubmit}>
                      <input
                        type="text"
                        value={tempPageNumber}
                        onChange={handlePageNumberChange}
                        onBlur={handlePageNumberSubmit}
                        autoFocus
                        className={styles.pageNumberInput}
                      />
                    </form>
                  ) : (
                    <>
                      <div className={styles.currentPage} onClick={handlePageNumberClick}>{pageNumber}</div>
                      <div className={styles.separator}>/</div>
                      <div className={styles.totalPages}>{numPages || '-'}</div>
                    </>
                  )}
                </div>
                <div className={styles.nextPage} onClick={handleNextPage}>&gt;</div>
              </div>
            </div>
          ) : (
            <div className={styles.headerControls}>
              <div className={styles.instructionText}>Start by holding right click and drag</div>
              <div className={styles.headerControlsBar}></div>
              <div className={styles.pageChangingControls}>
                <div className={styles.previousPage} onClick={handlePreviousPage}>&lt;</div>
                <div className={styles.pageNumber}>
                  <div className={styles.currentPage}>-</div>
                  <div className={styles.separator}>/</div>
                  <div className={styles.totalPages}>-</div>
                </div>
                <div className={styles.nextPage} onClick={handleNextPage}>&gt;</div>
              </div>
            </div>
          )}
          <div className={styles.headerControlsSave}>
            <button className={`${styles.headerButton} ${styles.saveButton}`} onClick={handleDownloadButtonClick}>Save</button>
          </div>
        </div>
        <button className={`${styles.headerButton} ${styles.helpButton}`}>?</button>
      </header>
      <div className={styles.pdfContainer}>
        {!pdfFile ? (
          <DropZone onFileInput={handleFileInput} isDragging={isDragging} />
        ) : (
          <div className={styles.documentContainer} ref={pdfContainerRef}>
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
        )}
      </div>
      {showSignatureMethodPopup && (
        <SignatureMethodPopup
          onClose={() => setShowSignatureMethodPopup(false)}
          onSelectMethod={handleSelectSignatureMethod}
        />
      )}
      {showSignaturePopup && (
        <SignaturePopup
          onClose={handleCloseSignaturePopup}
          onConfirm={handleConfirmation}
        />
      )}
      {showSuccessPopup && (
        <SuccessPopup
          message={successMessage}
          onSave={handleSaveAs}
          onContinue={handleContinue}
          isVerification={isVerification}
        />
      )}
      {isLoading && (
        <div className={styles.loadingOverlay}>
          <div className={styles.loadingSpinner}></div>
        </div>
      )}
    </div>
  );
}