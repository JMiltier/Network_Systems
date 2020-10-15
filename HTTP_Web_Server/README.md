# 🌐 HTTP-based Web Server
An HTTP web server that can handle multiple simultaneous requests from users. An HTTP **request** from a client is received, processed, and those results are sent back to the client as a **response**. HTTP requests consist of three substrings: request method (GET, HEAD, POST, etc.), request URL (separated by '/', treated as a relative path), and request version ('HTTP/x,y', where *x* & *y* are numbers).

### Program
A *C and Python Language* based program that sends TCP commands/requests to an HTTP server. The web server returns the response back to the client.

## ⚙️ SETUP
### CLI Commands

#### Web server testing
  - Launch the HTTP server within a commonly used web browser (i.e. Chome, Firefox) and direct requests to the server, port number, and `index.html` file. Example: [http://localhost:3000/index.html](http://localhost:3000/index.html).

## 📟 USE
#### HTTP User (client) Commands
The request consists of three parts (separated by spaces):
1. method (GET, POST, HEAD)
2. URL (directory)
3. HTTP version
Example: `GET /Protocols/rfc1945/rfc1945 HTTP/1.1`