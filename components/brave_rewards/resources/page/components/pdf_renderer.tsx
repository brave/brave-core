/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
import { useState, useCallback, useEffect, useRef } from 'react';
import * as React from 'react';
import { pdfjs, Document, Page } from 'react-pdf';
import 'react-pdf/dist/esm/Page/AnnotationLayer.css';
import pdfLogo from '../assets/pdfLogo.png';
import pdfMain from '../assets/pdfMain.png';
import styles from './ping-sign-pdf.module.css';
import { verifyPDF } from './pdf_verify';
import { signPdf } from './pdf_signer';

pdfjs.GlobalWorkerOptions.workerSrc = new URL(
  './pdfjs-dist-worker.js',
  import.meta.url,
).toString();

// interface TooltipProps {
//   content: string;
//   children: React.ReactNode;
// }

export interface SelectionCoords {
  startX: number;
  startY: number;
  endX: number;
  endY: number;
}

// const Tooltip: React.FC<TooltipProps> = ({ content, children }) => {
//   return (
//     <div className={styles.tooltipWrapper}>
//       {children}
//       <span className={styles.tooltipText}>{content}</span>
//     </div>
//   );
// };

interface SignatureTypePopupProps {
  onClose: () => void;
  onConfirm: (signatureName: string) => void;
}

const SignatureTypePopup: React.FC<SignatureTypePopupProps> = ({ onClose, onConfirm }) => {
  const [signatureName, setSignatureName] = useState<string>("John Doe");

  const handleConfirm = () => {
    if (signatureName) {
      onConfirm(signatureName);
    }
  };

  return (
    <div className={styles.popupOverlay}>
      <div className={styles.popupTypeContent}>
        <h2 className={styles.h2}>Choose a digital ID to sign with:</h2>
        <button className={styles.closeButton} onClick={onClose}>×</button>

        <div className={styles.typedSignature}>
          <h3 className={styles.h3}>{signatureName}</h3>
        </div>

        <input
          type="text"
          placeholder="Type your name"
          value={signatureName}
          onChange={(e) => setSignatureName(e.target.value)}
          className={styles.nameInput}
        />

        <div className={styles.typeButtons}>
          <button
            className={styles.confirmButton}
            onClick={handleConfirm}
          >
            Confirm signature
          </button>
        </div>
      </div>
    </div>
  );
};

interface SignaturePopupProps {
  onClose: () => void;
  onConfirm: (signature: string) => void;
}

const SignaturePopup: React.FC<SignaturePopupProps> = ({ onClose, onConfirm }) => {
  const [selectedSignature, setSelectedSignature] = useState<string | null>(null);

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
            <h3>{signatures.find(sig => sig.id === selectedSignature)?.name}</h3>
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

interface SignatureMethodPopupProps {
  onClose: () => void;
  onSelectMethod: (method: 'digitalID' | 'imageUpload') => void;
}

const SignatureMethodPopup: React.FC<SignatureMethodPopupProps> = ({ onClose, onSelectMethod }) => (
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

interface SuccessPopupProps {
  message: string;
  onSave: () => void;
  onContinue: () => void;
  isVerification: boolean;
}

const SuccessPopup: React.FC<SuccessPopupProps> = ({ message, onSave, onContinue, isVerification }) => (
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

interface AnimatedStatusProps {
  message: string;
  type: string;
}

const AnimatedStatus: React.FC<AnimatedStatusProps> = ({ message, type }) => (
  <div className={`${styles.animatedStatus} ${styles[type]} ${styles.visible}`}>
    <div className={styles.statusContent}>
      Status: <span className={styles.tex}>{message}</span>
    </div>
  </div>
);

export function PdfRenderer() {
  const [pdfFile, setPdfFile] = useState<Blob | null>(null);
  const [pdfFileName, setPdfFileName] = useState<string>('');
  const [numPages, setNumPages] = useState<number | null>(null);
  const [pdfBuff, setPdfBuff] = useState<Buffer | null>(null);
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const [isSelecting, setIsSelecting] = useState<boolean>(false);
  const [isSelectionEnabled, setIsSelectionEnabled] = useState<boolean>(false);
  const [selectionCoords, setSelectionCoords] = useState<SelectionCoords>({ startX: 1, startY: 1, endX: 1, endY: 1 });
  const [currentPageIndex, setCurrentPageIndex] = useState<number | null>(null);
  const [hsmPath, setHsmPath] = useState<string>('');
  const [pin, setPin] = useState<string>('');
  // const [selectedSignature, setSelectedSignature] = useState<string | null>(null);
  const [pageNumber, setPageNumber] = useState<number>(1);
  const [isEditingPageNumber, setIsEditingPageNumber] = useState<boolean>(false);
  const [tempPageNumber, setTempPageNumber] = useState<string>('');
  const [isDragging, setIsDragging] = useState<boolean>(false);
  const [showSignaturePopup, setShowSignaturePopup] = useState<boolean>(false);
  const [showSuccessPopup, setShowSuccessPopup] = useState<boolean>(false);
  const [successMessage, setSuccessMessage] = useState<string>('');
  const [showSignatureMethodPopup, setShowSignatureMethodPopup] = useState<boolean>(false);
  // const [selectedSignatureImage, setSelectedSignatureImage] = useState<string | null>(null);
  const [isVerification, setIsVerification] = useState<boolean>(false);
  const [isVerified, setIsVerified] = useState<boolean>(false);
  const [isVerificationFailed, setIsVerificationFailed] = useState<boolean>(false);
  const [statusMessage, setStatusMessage] = useState<string>('');
  const [isStatusVisible, setIsStatusVisible] = useState<boolean>(false);
  const [showTypeSignaturePopup, setShowTypeSignaturePopup] = useState<boolean>(false);
  const [statusType, setStatusType] = useState<string>('checking');
  const overlayCanvasRefs = useRef<HTMLCanvasElement[]>([]);
  const pdfCanvasRefs = useRef<HTMLCanvasElement[]>([]);
  const pdfContainerRef = useRef<HTMLDivElement>(null);
  const pageRefs = useRef<HTMLDivElement[]>([]);
  const fileInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    const promptHsmPath = async () => {
      // const path = prompt('Please enter the HSM path:');
      const path = "hardcoded-for-now";
      if (path) {
        setHsmPath(path);
      }
    };
    promptHsmPath();
  }, []);

  const handleFileInput = useCallback(
    async (event: React.ChangeEvent<HTMLInputElement> | React.DragEvent<HTMLDivElement>) => {
      const file = 'files' in event.target
        ? event.target.files?.[0]
        : (event as React.DragEvent<HTMLDivElement>).dataTransfer.files[0];
      if (file && file.type === 'application/pdf') {
        try {
          const arrayBuffer = await file.arrayBuffer();
          const buff = Buffer.from(arrayBuffer);
          setPdfBuff(buff);
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
    (event: React.DragEvent<HTMLDivElement>) => {
      event.preventDefault();
      setIsDragging(false);
      handleFileInput(event);
    },
    [handleFileInput]
  );

  const handleLogoClick = () => {
    if (!pdfFile) {
      fileInputRef.current?.click();
    }
  };

  const getMousePos = (canvas: HTMLCanvasElement, event: React.MouseEvent): { x: number; y: number } => {
    const rect = canvas.getBoundingClientRect();
    return {
      x: event.clientX - rect.left,
      y: event.clientY - rect.top,
    };
  };

  const isSelectionValid = (): boolean => {
    const { startX, startY, endX, endY } = selectionCoords;
    const width = Math.abs(endX - startX);
    const height = Math.abs(endY - startY);
    const minSize = 50; // Minimum size in pixels
    return width >= minSize && height >= minSize;
  };

  const handleMouseDown = (event: React.MouseEvent<HTMLCanvasElement>, pageIndex: number) => {
    if (!isSelectionEnabled) return;
    setIsSelecting(true);
    const canvas = overlayCanvasRefs.current[pageIndex];
    if (canvas) {
      const pos = getMousePos(canvas, event);
      setSelectionCoords({ startX: pos.x, startY: pos.y, endX: pos.x, endY: pos.y });
      setCurrentPageIndex(pageIndex);
    }
  };

  const handleMouseMove = (event: React.MouseEvent<HTMLCanvasElement>, pageIndex: number) => {
    if (!isSelecting || !isSelectionEnabled) return;
    const canvas = overlayCanvasRefs.current[pageIndex];
    if (canvas) {
      const pos = getMousePos(canvas, event);
      setSelectionCoords(prev => ({ ...prev, endX: pos.x, endY: pos.y }));
      drawSelection(pos.x, pos.y, pageIndex);
    }
  };

  const handleMouseUp = (pageIndex: number) => {
    if (!isSelectionEnabled) return;
    setIsSelecting(false);
    if (isSelectionValid()) {
      showEmbedSignConfirmation(pageIndex);
    } else {
      alert('Selected area is too small. Please select a larger area.');
      clearSelection(pageIndex);
    }
  };

  const handleCloseSignatureTypePopup = () => {
    setShowTypeSignaturePopup(false)
    setIsStatusVisible(false);
  }

  const handleCloseSignaturePopup = () => {
    setShowSignaturePopup(false);
    setIsStatusVisible(false);
  }

  const handleCloseSignatureMethodPopup = () => {
    setShowSignatureMethodPopup(false);
    setIsStatusVisible(false);
  }

  const drawSelection = (endX: number, endY: number, pageIndex: number) => {
    const { startX, startY } = selectionCoords;
    const canvas = overlayCanvasRefs.current[pageIndex];
    if (canvas) {
      const overlayCtx = canvas.getContext('2d');
      if (overlayCtx) {
        clearOverlay(pageIndex);
        overlayCtx.strokeStyle = 'blue';
        overlayCtx.lineWidth = 2;
        overlayCtx.strokeRect(startX, startY, endX - startX, endY - startY);
      }
    }
  };

  const clearOverlay = (pageIndex: number) => {
    const canvas = overlayCanvasRefs.current[pageIndex];
    if (canvas) {
      const overlayCtx = canvas.getContext('2d');
      if (overlayCtx) {
        overlayCtx.clearRect(0, 0, canvas.width, canvas.height);
      }
    }
  };

  const clearAllSelections = () => {
    overlayCanvasRefs.current.forEach((canvas, index) => {
      if (canvas) {
        clearOverlay(index);
      }
    });
  };

  const showEmbedSignConfirmation = (pageIndex: number) => {
    const confirmation = window.confirm("Do you want to embed the signature in the selected area?");
    if (confirmation) {
      setCurrentPageIndex(pageIndex);
      sendSignRequest();
    } else {
      clearSelection(pageIndex);
    }
  };

  const clearSelection = (pageIndex: number) => {
    clearOverlay(pageIndex);
    setSelectionCoords({ startX: 1, startY: 1, endX: 1, endY: 1 });
  };

  const sendSignRequest = async () => {
    if (!pdfBuff || currentPageIndex === null) return;

    setIsLoading(true);
    setStatusMessage('Signing ...');
    setStatusType('checking');
    setIsStatusVisible(true);

    try {
      const signedPdfBuffer = await signPdf(
        pdfBuff,
        currentPageIndex,
        selectionCoords,
        hsmPath,
        pin,
      );

      setPdfFile(new Blob([signedPdfBuffer], { type: 'application/pdf' }));
      setPdfBuff(Buffer.from(signedPdfBuffer));
      setStatusMessage('Signature Complete');
      setStatusType('success');
      setIsStatusVisible(true);
      setTimeout(() => {
        setIsStatusVisible(false);
      }, 3000);
      setIsSelectionEnabled(false);
      setShowSuccessPopup(true);
      setSuccessMessage(`Your document has been signed`);
    } catch (error) {
      console.error('Signing error:', error);
      setStatusMessage('Error');
      setStatusType('error');
      setIsStatusVisible(true);
      setTimeout(() => {
        setIsStatusVisible(false);
        alert(`Error: ${error}`);
      }, 3000);
    } finally {
      setIsLoading(false);
      clearAllSelections();
      setPin(''); // Clear the PIN after use
    }
  };

  const handleSignButtonClick = useCallback(() => {
    if (!pdfBuff) {
      alert('Please upload a PDF first');
      return;
    }
    const enteredPin = prompt('Please enter your PIN:');
    if (enteredPin) {
      setPin(enteredPin);
      setStatusMessage('Preparing to sign ...');
      setStatusType('checking');
      setIsStatusVisible(true);
      setShowSignatureMethodPopup(true);
    } else {
      alert('PIN is required to sign the document');
    }
  }, [pdfBuff]);

  const handleVerifyButtonClick = useCallback(async () => {
    if (!pdfBuff) {
      alert('Please upload a PDF first');
      return;
    }
    setStatusMessage('Checking ...');
    setStatusType('checking');
    setIsStatusVisible(true);

    try {
      const isVerified = await verifyPDF(pdfBuff);
      if (isVerified) {
        setStatusMessage('Verification Successful');
        setStatusType('success');
        setIsVerified(true);
        setIsVerificationFailed(false);
      } else {
        throw new Error('Verification failed');
      }
    } catch (error) {
      setStatusMessage('Verification Failed');
      setStatusType('error');
      setIsVerified(false);
      setIsVerificationFailed(true);
    } finally {
      setIsStatusVisible(true);
      setTimeout(() => {
        setIsStatusVisible(false);
        if (isVerified) {
          setSuccessMessage('Document verification successful!');
          setIsVerification(true);
          setShowSuccessPopup(true);
        }
      }, 3000);
    }
  }, [pdfBuff, isVerified]);

  const handleDownloadButtonClick = () => {
    if (pdfFile) {
      const link = document.createElement('a');
      link.href = URL.createObjectURL(pdfFile);
      link.download = 'signed_document.pdf';
      link.click();
    }
  };

  const onDocumentLoadSuccess = useCallback(({ numPages }: { numPages: number }) => {
    setNumPages(numPages);
    setIsVerified(false);
    setPageNumber(1);
  }, []);

  const onPageLoadSuccess = useCallback((pageIndex: number) => {
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

  const scrollToPage = (pageNum: number) => {
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

  const handlePageNumberChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setTempPageNumber(e.target.value);
  };

  const handlePageNumberSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    const newPageNumber = parseInt(tempPageNumber, 10);
    if (!isNaN(newPageNumber) && newPageNumber >= 1 && newPageNumber <= (numPages || 0)) {
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
    const handleKeyDown = (event: KeyboardEvent) => {
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

  const DropZone: React.FC<{ onFileInput: (event: React.ChangeEvent<HTMLInputElement>) => void, isDragging: boolean }> = ({ onFileInput, isDragging }) => {
    const fileInputRef = useRef<HTMLInputElement>(null);

    const handleLogoClick = () => {
      fileInputRef.current?.click();
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
            src={pdfMain}
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
            src={pdfLogo}
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
                  <button
                    className={`${styles.headerButton} ${isVerified ? styles.verified : ""} ${isVerificationFailed ? styles.notVerified : ""}`}
                    onClick={handleVerifyButtonClick}
                  >
                    Verify document
                  </button>
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
                  <div key={`page_${index + 1}`} style={{ position: 'relative', marginBottom: '20px' }} ref={(el) => (pageRefs.current[index] = el!)}>
                    <Page
                      pageNumber={index + 1}
                      renderTextLayer={false}
                      renderMode="canvas"
                      onLoadSuccess={() => onPageLoadSuccess(index)}
                      canvasRef={(el) => (pdfCanvasRefs.current[index] = el!)}
                      loading={<div>Loading page...</div>}
                    />
                    <canvas
                      id={`overlayCanvas_${index}`}
                      ref={(el) => (overlayCanvasRefs.current[index] = el!)}
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
          onClose={handleCloseSignatureMethodPopup}
          onSelectMethod={(method) => {
            setShowSignatureMethodPopup(false);
            if (method === 'digitalID') {
              setShowSignaturePopup(true);
            } else {
              setShowTypeSignaturePopup(true);
            }
          }}
        />
      )}
      {showSignaturePopup && (
        <SignaturePopup
          onClose={handleCloseSignaturePopup}
          // onConfirm={(signature) => {
          //   setSelectedSignature(signature);
          //   setShowSignaturePopup(false);
          //   setIsSelectionEnabled(true);
          // }}
          onConfirm={() => {
            setShowSignaturePopup(false);
            setIsSelectionEnabled(true);
          }}
        />
      )}
      {showTypeSignaturePopup && (
        <SignatureTypePopup
          onClose={handleCloseSignatureTypePopup}
          // onConfirm={(signatureName) => {
          //   setSelectedSignatureImage(signatureName);
          //   setIsSelectionEnabled(true);
          //   setShowTypeSignaturePopup(false);
          // }}
          onConfirm={() => {
            setIsSelectionEnabled(true);
            setShowTypeSignaturePopup(false);
          }}
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