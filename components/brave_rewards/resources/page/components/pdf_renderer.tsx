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

pdfjs.GlobalWorkerOptions.workerSrc = new URL(
  './pdfjs-dist-worker.js',
  import.meta.url,
).toString();

const ws = new WebSocket('ws://localhost:5000');

export function PdfRenderer() {
  const [pdfFile, setPdfFile] = useState(null);
  const [numPages, setNumPages] = useState(null);
  const [pdfBuff, setPdfBuff] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [verificationResult, setVerificationResult] = useState(null);
  const [hsmPath, setHsmPath] = useState('');
  const [selectedSignature, setSelectedSignature] = useState(null);
  const [showConfirmation, setShowConfirmation] = useState(false);

  const showToast = useCallback((message, type) => {
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerText = message;
    document.body.appendChild(toast);
    setTimeout(() => {
      toast.classList.add('fade-out');
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
      const file = event.target.files[0];
      if (file) {
        try {
          const arrayBuffer = await file.arrayBuffer();
          setPdfBuff(arrayBuffer);
          setPdfFile(new Blob([arrayBuffer], { type: 'application/pdf' }));
          setVerificationResult(null);
        } catch (error) {
          console.error('Error reading PDF file:', error);
          showToast('Error reading PDF file. Please try again.', 'error');
        }
      }
    },
    [showToast],
  );

  const promptHsmPath = () => {
    const path = prompt('Enter HSM path:');
    if (path) setHsmPath(path);
    return path;
  };

  const selectSignature = () => {
    // TODO: To be fetched from extension
    setSelectedSignature('mock_signature');
  };

  const showConfirmationPreview = () => {
    setShowConfirmation(true);
  };

  const handleConfirmation = (confirmed) => {
    if (confirmed) {
      sendSignRequest();
    }
    setShowConfirmation(false);
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

    const path = promptHsmPath();
    if (!path) return;

    selectSignature();
    showConfirmationPreview();
  }, [pdfBuff, showToast]);

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
  }, []);

  const ConfirmationDialog = ({ onConfirm }) => (
    <div className="confirmation-dialog">
      <h3>Confirm Signature</h3>
      <p>Are you sure you want to sign the document?</p>
      <button onClick={() => onConfirm(true)}>Confirm</button>
      <button onClick={() => onConfirm(false)}>Cancel</button>
    </div>
  );

  return (
    <div
      className="App"
      style={{ padding: '20px', fontFamily: 'Arial, sans-serif' }}
    >
      <div
        id="controls"
        style={{
          display: 'flex',
          justifyContent: 'center',
          marginBottom: '20px',
        }}
      >
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
          style={{
            padding: '10px',
            marginRight: '10px',
            backgroundColor: '#007BFF',
            color: '#FFF',
            border: 'none',
            borderRadius: '5px',
          }}
          disabled={isLoading}
        >
          Sign
        </button>
        <button
          id="verifyButton"
          onClick={handleVerifyButtonClick}
          style={{
            padding: '10px',
            marginRight: '10px',
            backgroundColor: '#28A745',
            color: '#FFF',
            border: 'none',
            borderRadius: '5px',
          }}
          disabled={isLoading}
        >
          Verify
        </button>
        <button
          id="downloadButton"
          onClick={handleDownloadButtonClick}
          style={{
            padding: '10px',
            backgroundColor: '#FFC107',
            color: '#FFF',
            border: 'none',
            borderRadius: '5px',
          }}
          disabled={!pdfFile || isLoading}
        >
          Download
        </button>
      </div>
      {verificationResult !== null && (
        <div
          id="verificationResult"
          style={{ marginBottom: '20px', textAlign: 'center' }}
        >
          <h3>Verification Result:</h3>
          <p>
            {verificationResult
              ? 'Document is verified'
              : 'Document verification failed'}
          </p>
        </div>
      )}
      {showConfirmation && (
        <ConfirmationDialog onConfirm={handleConfirmation} />
      )}
      <div
        id="pdfContainer"
        style={{
          maxHeight: '80vh',
          overflowY: 'auto',
          border: '1px solid #ccc',
          padding: '10px',
          backgroundColor: '#f9f9f9',
        }}
      >
        <Document file={pdfFile} onLoadSuccess={onDocumentLoadSuccess}>
          {Array.from(new Array(numPages), (el, index) => (
            <Page
              key={`page_${index + 1}`}
              pageNumber={index + 1}
              width={600}
            />
          ))}
        </Document>
      </div>
      {isLoading && (
        <div className="loading-overlay">
          <div className="loading-spinner"></div>
        </div>
      )}
      <style jsx>{`
        .loading-overlay {
          position: fixed;
          top: 0;
          left: 0;
          right: 0;
          bottom: 0;
          background-color: rgba(0, 0, 0, 0.5);
          display: flex;
          justify-content: center;
          align-items: center;
          z-index: 1001;
        }
        .loading-spinner {
          border: 5px solid #f3f3f3;
          border-top: 5px solid #3498db;
          border-radius: 50%;
          width: 50px;
          height: 50px;
          animation: spin 1s linear infinite;
        }
        @keyframes spin {
          0% {
            transform: rotate(0deg);
          }
          100% {
            transform: rotate(360deg);
          }
        }
        .toast {
          position: fixed;
          top: 20px;
          right: 20px;
          padding: 10px 20px;
          border-radius: 4px;
          color: white;
          opacity: 0.9;
          transition: opacity 0.3s ease;
          z-index: 1002;
        }
        .toast.success {
          background-color: #4caf50;
        }
        .toast.error {
          background-color: #f44336;
        }
        .toast.info {
          background-color: #2196f3;
        }
        .toast.fade-out {
          opacity: 0;
        }
        .confirmation-dialog {
          position: fixed;
          top: 50%;
          left: 50%;
          transform: translate(-50%, -50%);
          background-color: white;
          padding: 20px;
          border-radius: 8px;
          box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
          z-index: 1003;
        }
      `}</style>
    </div>
  );
}
