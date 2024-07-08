export const signDocument = () => {
  const url = "ping://rewards";
  chrome.tabs.update({ url: url });
}
export class SigningService {
  private socket: WebSocket | null = null;
  private readonly serverUrl: string;
  private reconnectInterval: number;

  // TODO: Hardcoded Certificate 
  private static readonly DUMMY_CERTIFICATE = `-----BEGIN CERTIFICATE-----
MIIDazCCAlOgAwIBAgIUBnkh4kN8iArjYCdazBmdoOhBjIUwDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMzA3MDgxMjAwMDBaFw0yNDA3
MDcxMjAwMDBaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQC8mJwV+5oBwMNS9JfbL5VzL/aVgDVPYC+zCghxA9wM
HXDJ4OsD7xrxS6FWBj7FGsGDqcMJVRNyeX1Mjs+z9VCw7Fo1NxY0BIi5MfkCPq9c
s10UNRrWPH4BZ7Z5S5KMrjKyiEAM8TmPczuzRQYdLYLrLe9vxXOaR2mG45G/hy50
fQ5e/ykaMq4kG38cchh9Ywe+RX6YjlWzPUwnTmyG3JbAyDZjqz/3h5JGM7aRpCgk
Wwd5J57mTaIaFztBgNxQIshkX4ztbYmtWKYNGaUOVSNdS7bPNd34GxH9e8lC6eXz
ZZ+PD9JsrSWWy2hy5r1UljrBwfQZl/QaOjZh2/4cSMcTAgMBAAGjUzBRMB0GA1Ud
DgQWBBTHZ9+wFAkC89yWPE3/UY+xYbtPIzAfBgNVHSMEGDAWgBTHZ9+wFAkC89yW
PE3/UY+xYbtPIzAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCU
9gRbhTU1pAIfAgqN2nQMeTN2hGAUH2cPvNz5ZUq5ksRCwFU8DAtRSE4jBqvZuCnB
1fYCkbYx28OkztLcK0mgFVJI13bQ+7acwjMZD0kc0LTf0TYeOoKKzCuKjW5g+ULy
HHKQFMW+jXHHAWiQBLRvh+XotovWPAPJ5X4xdUwZ5OqHC1TuBJxXYnmVzvWY2t7A
1q1Wz0o4nTLWTBq20vD9XTxlC8/pu8r+cXzN6b3jQz1Cq6bYTyH4Nsg+iBRPhYJb
9L6m7rmnJQ6EkNLCLFGSh/Ug4q14p8kTDXnZnUcBCIzpY7WFdRn0JO1XY1hGWfPp
uDOKxMwuwy3YDZL1jUOu
-----END CERTIFICATE-----`;

  constructor(serverUrl: string = 'ws://localhost:5001', reconnectInterval: number = 5000) {
    this.serverUrl = serverUrl;
    this.reconnectInterval = reconnectInterval;
    this.connectWebSocket();
  }

  private connectWebSocket(): void {
    this.socket = new WebSocket(this.serverUrl);

    this.socket.onopen = this.handleOpen.bind(this);
    this.socket.onmessage = this.handleMessage.bind(this);
    this.socket.onclose = this.handleClose.bind(this);
    this.socket.onerror = this.handleError.bind(this);
  }

  private handleOpen(): void {
    console.log('Connected to WebSocket server');
  }

  private handleMessage(event: MessageEvent): void {
    const message = JSON.parse(event.data);
    
    if (message.action === 'getCertAndSign') {
      this.handleGetCertAndSign(message.data);
    }
  }

  private handleClose(event: CloseEvent): void {
    console.log('WebSocket connection closed:', event.reason);
    setTimeout(() => this.connectWebSocket(), this.reconnectInterval);
  }

  private handleError(error: Event): void {
    console.error('WebSocket error:', error);
  }

  private handleGetCertAndSign(data: { mdhash: string }): void {
    const { mdhash } = data;

    // TODO: Hardcoded signature
    const dummySignature = 'dummy_signature_' + mdhash;

    const response = {
      action: 'certAndSignResponse',
      data: {
        cert: SigningService.DUMMY_CERTIFICATE,
        signedHash: dummySignature
      }
    };

    this.sendMessage(response);
  }

  public sendMessage(message: any): void {
    if (this.socket && this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(message));
    } else {
      console.error('WebSocket is not connected');
    }
  }
}