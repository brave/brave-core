/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
// @ts-nocheck
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
// @ts-nocheck
import React, { useState, useCallback, useEffect } from 'react';
import { pdfjs, Document, Page } from 'react-pdf';
import 'react-pdf/dist/esm/Page/AnnotationLayer.css';
import styles from './ping-sign-pdf.module.css';

pdfjs.GlobalWorkerOptions.workerSrc = new URL(
  './pdfjs-dist-worker.js',
  import.meta.url,
).toString();

const ws = new WebSocket('ws://localhost:5000');

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
      <div className={styles.popupContent}>
        <h2>Choose a digital ID to sign with:</h2>
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
              <span>{sig.name}</span>
              <span className={styles.issueDate}>Issued: {sig.issueDate}</span>
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

export function PdfRenderer() {
  const [pdfFile, setPdfFile] = useState(null);
  const [pdfFileName, setPdfFileName] = useState('');
  const [numPages, setNumPages] = useState(null);
  const [pdfBuff, setPdfBuff] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [verificationResult, setVerificationResult] = useState(null);
  const [hsmPath, setHsmPath] = useState('');
  const [selectedSignature, setSelectedSignature] = useState(null);
  const [showConfirmation, setShowConfirmation] = useState(false);
  const [pageNumber, setPageNumber] = useState(1);
  const [isEditingPageNumber, setIsEditingPageNumber] = useState(false);
  const [tempPageNumber, setTempPageNumber] = useState('');
  const [isDragging, setIsDragging] = useState(false);
  const [showSignaturePopup, setShowSignaturePopup] = useState(false);

  const showToast = useCallback((message, type) => {
    const toast = document.createElement('div');
    toast.className = `${styles.toast} ${styles[`toast${type.charAt(0).toUpperCase() + type.slice(1)}`]}`;
    toast.innerText = message;
    document.body.appendChild(toast);
    setTimeout(() => {
      toast.classList.add(styles.toastFadeOut);
      toast.addEventListener('transitionend', () => toast.remove());
    }, 3000);
  }, []);

  useEffect(() => {
    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.action === 'signed') {
        const signedPdfBuffer = new Uint8Array(
          Buffer.from(data.data, 'base64'),
        );
        setPdfFile(new Blob([signedPdfBuffer], { type: 'application/pdf' }));
        setPdfBuff(signedPdfBuffer);
        showToast('Document signed successfully!', 'success');
        setIsLoading(false);
      } else if (data.action === 'verified') {
        setVerificationResult(data.verified);
        setIsLoading(false);
        if (data.verified) {
          showToast('Document verification successful!', 'success');
        } else {
          showToast('Document verification failed.', 'error');
        }
      } else if (data.action === 'error') {
        console.error('Server error:', data.message);
        showToast(`Error: ${data.message}`, 'error');
        setIsLoading(false);
      }
    };

    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      showToast('Connection error. Please try again later.', 'error');
    };
  }, [showToast]);

  const handleFileInput = useCallback(
    async (event) => {
      const file = event.target.files ? event.target.files[0] : event.dataTransfer.files[0];
      if (file && file.type === 'application/pdf') {
        try {
          const arrayBuffer = await file.arrayBuffer();
          setPdfBuff(arrayBuffer);
          setPdfFile(new Blob([arrayBuffer], { type: 'application/pdf' }));
          setPdfFileName(file.name);
          setVerificationResult(null);
        } catch (error) {
          console.error('Error reading PDF file:', error);
          showToast('Error reading PDF file. Please try again.', 'error');
        }
      } else {
        showToast('Please upload a valid PDF file.', 'error');
      }
    },
    [showToast]
  );

  const handleDrop = useCallback(
    (event) => {
      event.preventDefault();
      setIsDragging(false);
      handleFileInput(event);
    },
    [handleFileInput]
  );

  const promptHsmPath = () => {
    const path = prompt('Enter HSM path:');
    if (path) setHsmPath(path);
    return path;
  };

  const sendSignRequest = () => {
    setIsLoading(true);
    const hardcodedCoords = { startX: 100, startY: 100, endX: 200, endY: 200 };
    ws.send(
      JSON.stringify({
        action: 'sign',
        data: {
          pdfBuffer: Array.from(new Uint8Array(pdfBuff)),
          pageIndex: 0,
          selectionCoords: hardcodedCoords,
          hsmPath,
          signatureId: selectedSignature,
        },
      }),
    );
  };

  const handleSignButtonClick = useCallback(() => {
    if (!pdfBuff) {
      showToast('Please upload a PDF first', 'error');
      return;
    }
    setShowSignaturePopup(true);
  }, [pdfBuff, showToast]);

  const handleCloseSignaturePopup = () => {
    setShowSignaturePopup(false);
  };

  const handleConfirmation = (signature) => {
    setSelectedSignature(signature);
    setShowSignaturePopup(false);
    const path = promptHsmPath();
    if (!path) return;
    sendSignRequest();
  };

  const handleVerifyButtonClick = useCallback(() => {
    if (!pdfBuff) {
      showToast('Please upload a PDF first', 'error');
      return;
    }
    setIsLoading(true);
    ws.send(
      JSON.stringify({
        action: 'verify',
        data: {
          Buff: Array.from(new Uint8Array(pdfBuff)),
        },
      }),
    );
  }, [pdfBuff, showToast]);

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
    setPageNumber(1);
  }, []);

  const handlePreviousPage = useCallback(() => {
    setPageNumber((prevPageNumber) => Math.max(prevPageNumber - 1, 1));
  }, []);

  const handleNextPage = useCallback(() => {
    setPageNumber((prevPageNumber) => 
      numPages ? Math.min(prevPageNumber + 1, numPages) : prevPageNumber
    );
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
    }
    setIsEditingPageNumber(false);
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

  const DropZone = ({ onFileInput, isDragging }) => (
    <div 
      className={`${styles.dropZone} ${isDragging ? styles.dragging : ''}`}
      onDragOver={(e) => e.preventDefault()}
      onDragEnter={() => setIsDragging(true)}
      onDragLeave={() => setIsDragging(false)}
      onDrop={handleDrop}
    >
      <div className={styles.pdfIcon}>📄</div>
      <p className={styles.pdfText}>Choose a PDF file to add your digital signature</p>
      <label htmlFor="fileInput" className={styles.addFileButton}>
        + Add file
      </label>
      <input
        id="fileInput"
        type="file"
        accept=".pdf"
        onChange={onFileInput}
        style={{ display: 'none' }}
      />
    </div>
  );

  return (
    <div className={styles.app}>
      <header className={styles.header}>
        <div className={styles.navBar}>
          <div className={styles.logo}>📄</div>
          <div className={styles.pdfFileName}>{pdfFileName}</div>
          {pdfFile ? (
            <div className={styles.headerControls}>
              <button className={styles.headerButton} onClick={handleSignButtonClick}>Add signature</button>
              <div className={styles.headerControlsBar}></div>
              <button className={styles.headerButton} onClick={handleVerifyButtonClick}>Verify document</button>
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
          <Document file={pdfFile} onLoadSuccess={onDocumentLoadSuccess}>
            <Page
              pageNumber={pageNumber}
              width={600}
            />
          </Document>
        )}
      </div>
      {verificationResult !== null && (
        <div className={styles.verificationResult}>
          <h3>Verification Result:</h3>
          <p>
            {verificationResult
              ? 'Document is verified'
              : 'Document verification failed'}
          </p>
        </div>
      )}
      {showSignaturePopup && (
        <SignaturePopup
          onClose={handleCloseSignaturePopup}
          onConfirm={handleConfirmation}
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