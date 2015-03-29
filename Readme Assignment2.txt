Assignment 2 Readme 


Server :

1. Compile the server using command:
gcc -o server server.c

2. To start the server give port number, mapping file name and redirect filename :
./server -p <port number> -f <mapping file> -r <redirect file>



Client:

1. Compile client using command:
gcc -o client client.c

2. To request a hostname from the server giving IP address as input:
./client -s <IP address of server> -p <Port number of server> -r 0 -d <Input IP address> -t <Timeout>  -n <Sequence number>
* use localhost 127.0.0.1 as IP address

e.g ./client -s 127.0.0.1 -p 5000 -r 1 -d google.com -t 20 -n 10

3. To request a IP address from the server giving Hostname as input:
./client -s <IP address of server> -p <Port number of server> -r 1 -d <Input Hostname> -t <Timeout>  -n <Sequence number>

e.g ./client -s 127.0.0.1 -p 5000 -r 0 -d 128.45.6.8 -t 20 -n 100

4. To terminate the server: 
./client -s <IP address of server> -p <Port number of server> -r 6 -d -t <Timeout>  -n <Sequence number>

e.g ./client -s 127.0.0.1 -p 5000 -r 6 -d  -t 2 -n 100

5. To see time out function make value of time out (-t) less than 3.


