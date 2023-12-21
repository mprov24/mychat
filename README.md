# Build
Build all: `make`

Build server: `make server`

Build client: `make cclient`

# Usage

Run server: `./server [optional port number]`

Run client: `./cclient handle server-name server-port`

<br>

Client commands:
- Unicast: `%M destination-handle [text]`
- Broadcast: `%B [text]`
- Multicast: `%C num-handles destination-handle destination-handle [destination-handle] [text]`
- List clients: `%L`
- Exit: `%E`

# Example - localhost (127.0.0.1) with 2 clients

Terminal 1 (server)

1. `./server 12345`

<br>

Terminal 2 (client 1)

1. `./cclient client1 localhost 12345` OR `./cclient client1 127.0.0.1 12345`
2. `%M client2 hello client 2!`

<br>

Terminal 3 (client 2)

1. `./cclient client2 localhost 12345`
   
`hello client 2!`

# Files
- processPDU.c: application layer PDU implementation
- networks.c: server and client shared code
- safeUtil.c: handling for mallocs and recv()/send() wrapper
