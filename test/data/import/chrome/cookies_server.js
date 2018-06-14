const http = require('http')
const port = 8080

const requestHandler = (request, response) => {
  console.log(request.url)
  // Cookie needs Expires or Max-Age attribute to be a persistent
  // cookie; otherwise, the browser will treat it as an ephemeral
  // session cookie and may delete it when the browser is closed.
  //
  // The maximum allowable Expires date is used to avoid any potential
  // issues with the test browser clearing the cookie upon load if the
  // tests are run after the cookie's expiration date. The choice of
  // date and the decision to use an absolute date with Expires rather
  // than a relative one with Max-Age are based on this Stack Overflow
  // answer: https://stackoverflow.com/a/22479460.
  response.setHeader('Set-Cookie', ['test=test; Expires=Tue, 19 Jan 2038 03:14:07 GMT'])
  response.end('Cookie set')
}

const server = http.createServer(requestHandler)

server.listen(port, (err) => {
  if (err) {
    return console.log('Could not start server: ', err)
  }

  console.log(`server is listening on ${port}`)
})
