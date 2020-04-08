const $<paymentreq>_CLOSED = "closed";
const $<paymentreq>_CREATED = "created";
const $<paymentreq>_INTERACTIVE = "interactive";

Object.defineProperty(window, '$<paymentreqcallback>', {
  value: {}
});

class $<paymentresponsedetails> {
  constructor (orderId) {
    this.orderId = orderId
  }
}

class $<paymentresponse> {
  constructor (orderId) {
    this.details = new $<paymentresponsedetails>(orderId)
    this.methodName = "bat"
  }
}

Object.defineProperty($<paymentreqcallback>, 'paymentreq_postCreate', {
  value:
    function (orderId, errorName, errorMessage) {
      if (errorName.length == 0) {
        $<paymentreqcallback>.resolve(new $<paymentresponse>(orderId));
        return;
      }
      $<paymentreqcallback>.reject(new DOMException(errorMessage, errorName));
    }
})

class $<paymentreq> {
  constructor (methodData, details) {
      this.methodData = JSON.stringify(methodData)
      this.details = JSON.stringify(details)
      this.state = $<paymentreq>_CREATED
  }

  canMakePayment() {
    const methodData = JSON.parse(this.methodData)
    const details = JSON.parse(this.details)
    const state = this.state
    return new Promise(
      function (resolve, reject) {
        if (state != $<paymentreq>_CREATED) {
          reject(new DOMException("canMakePayment error", "InvalidStateError"))
          return
        }
        if (!methodData.some(e => e.supportedMethods === 'bat')) {
          resolve(false)
          return
        }
        resolve(true)
        return
      }
    )
  }

  show() {
    this.state = $<paymentreq>_INTERACTIVE
    const methodData = this.methodData
    const details = this.details
    return new Promise(
      function (resolve, reject) {
        $<paymentreqcallback>.resolve = resolve
        $<paymentreqcallback>.reject = reject
        webkit.messageHandlers.PaymentRequest.postMessage({ name: 'payment-request-show', methodData: methodData, details: details })
      }
    )
  }
}

Object.defineProperty(window, 'PaymentRequest', {
  value: $<paymentreq>
})
