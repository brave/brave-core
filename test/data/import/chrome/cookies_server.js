const http = require('http')
const port = 8080

const requestHandler = (request, response) => {
  console.log(request.url)
  response.setHeader('Set-Cookie', ['test=test'])
  response.end('Cookie set')
}

const server = http.createServer(requestHandler)

server.listen(port, (err) => {
  if (err) {
    return console.log('Could not start server: ', err)
  }

  console.log(`server is listening on ${port}`)
})