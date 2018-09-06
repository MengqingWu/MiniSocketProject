# MiniSocketProject
a mini socket project for kpix daq


## Compile

```
g++ myserver_udp.cpp -o server
g++ myclient_udp.cpp -o client
```

## Usage

*  You first turn on your server/listener:
```
./server 8099 # it is an naive local listener
```

*  Then you turn on your client to send data to the port your listener is on:
```
./client localhost 8099
```

## Note from 2017-Nov

** 2017 Nov 20 mengqing Wu

If binding failed: address already in use, how to diagnostic?

try:
```
$netstat -a | grep <portno>
```

if you have output, means this portal is in use, then try:
```
$netstat -tulpn 
```

this will give you the process id and the programe which is using this particular port.
