Object.defineProperty(window, '$<paymentreqcallback>', {
  value: {}
});

Object.defineProperty($<paymentreqcallback>, 'paymentreq_postCreate', {
  value:
    function (response, errorName, errorMessage) {
      if (errorName.length == 0) {
        $<paymentreqcallback>.resolve(response);
        return;
      }
      $<paymentreqcallback>.reject(new DOMException(errorMessage, errorName));
    }
})

class $<paymentreq> {
  constructor (methodData, details) {
      this.methodData = JSON.stringify(methodData)
      this.details = JSON.stringify(details)
  }

  canMakePayment() {
    return true;
  }

  show() {
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
