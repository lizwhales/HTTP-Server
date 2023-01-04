# HTTP-Server
A simple HTTP server that responds to a limited set of GET requests, returning valid response headers for a range of files and paths. Examples of content that needs to be served are:

| Content        | File Extension|
| ------------- |:--------------:|
| HTML        | .html            |
| JPEG        | .jpg             |
| CSS         | .css             |
| JavaScript  | .js              |


The server program must uses the following command line arguments:
⋅⋅* protocol number: 4 for IPv4 and 6 for IPv6
⋅⋅* port number
⋅⋅* string path to root web directory

The port number will be the port the server will listen for connections on. The string path to the root
web directory points to the directory that contains the content that is to be served. For example:
```/home/comp30023/website```

All paths requested by a client should b e treated as relative to the path specifed on the command line.
For example, if a request was received with the path ```/css/style.css``` it would be expected to be found
at:

```/home/comp30023/website/css/style.css```

The server supports responding with either a 200 response containing the requested file, or a 404
response if the requested file is not found. Response headers are valid and include:
⋅⋅*Http Status
⋅⋅*Content-type

## Running the Program/ Command line arguments

To run this server program on your VM prompt, type:
```./server [protocol number] [port number] [path to web root]```
where:
⋅⋅*[protocol number] is ```4 for IPv4 or 6 for IPv6```
⋅⋅*[port number] is a valid port numb er (e.g., ```8080```), and
⋅⋅*[path to web root] is a valid absolute path (e.g., ```/home/comp30023/website```)
