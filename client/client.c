//client [-s<server-ip>] -p <port> -r <req-type> -d <data> [-t <timeout>] [-n <seq-no>]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>   
#include <arpa/inet.h>
//#include"utils.h"

#define HEADER_LEN 8
// Function to add padding to 32 bit boundary
char * 
payload_padding(char *pld, int pld_len, int *new_pld_len)
{

    char *temp_pld = pld;
    int bits_to_pad = 32 - (pld_len % 32); 
    *new_pld_len = pld_len + bits_to_pad;
    int j;
    for (j=0; j<bits_to_pad; j++) {
        strcat((temp_pld+pld_len+j),"\0");
    } 

    return temp_pld;
}

int main(int argc, char *argv[])
{
    int portno, req_type, timeout, seq_no, sockfd, new_pld_len;
    char *server_ip, *data;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    socklen_t serv_len;
    unsigned char buffer[256];
    unsigned char buffer_i[256];
    int i;
    
    data = "\0";
    timeout = 0;
    
    if (argc < 9) {
	fprintf(stderr,"ERROR, Arguments missing in the executable\n");
	exit(1);
    }		
 // Storing arguments from console to variables
    for (i=0; i<argc; i++) {
	if (strcmp(argv[i],"-s") == 0) {
	    server_ip = argv[i+1];
	  //  printf("server_ip: %s\n", server_ip);
	    i=i+1; 
	}
	else if (strcmp(argv[i],"-p") == 0) {
	    portno = atoi(argv[i+1]);
        //printf("portno: %d\n", portno);
	    i=i+1;
	}
	else if (strcmp(argv[i],"-r") == 0) {
	    req_type = atoi(argv[i+1]);;
	    //printf("req_type: %d\n", req_type);
	    i=i+1;
	}
	else if (strcmp(argv[i],"-d") == 0){
	       data = argv[i+1];
	    //printf("data: %s\n", data);
	    i=i+1;
	}
	else if (strcmp(argv[i],"-t") == 0) {
	       timeout = atoi(argv[i+1]);
	    //printf("timeout: %d\n", timeout);
	    i=i+1;
	}
	else if (strcmp(argv[i],"-n") == 0) {
	    seq_no = atoi(argv[i+1]);
	    //printf("seq_no: %d\n", seq_no);
	    i=i+1;
	}
    } 
    
     // Socket creation

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr,"ERROR, Error opening socket\n");
    }   
 
    server = gethostbyname(server_ip);
    if (server == NULL) {
	fprintf(stderr,"ERROR, No such host present\n");
	exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    
    inet_pton(AF_INET, server_ip, &(serv_addr.sin_addr));
    serv_addr.sin_port = htons(portno);
    serv_len = sizeof(serv_addr);

    //Creating 2 bufferes one for sending packet and other for recieving input packet  
    
    bzero(buffer,256);
    bzero(buffer_i,256);

    unsigned char *ptr = buffer;
    unsigned short unused = htons(0);
    unsigned short prot = htons(640);
    unsigned short len = 0;
    struct in_addr ip_add;
    
    // creating packet to be send to server 
    
    memcpy(ptr, &prot, 2);  
    memcpy(ptr+2, &seq_no, 2);
    memcpy(ptr+4, &unused,1);
    
    if(req_type == 1){
        int pld_len;
        char *pld_with_32bitPadding = payload_padding(data, strlen(data), &pld_len);
        len = htons(HEADER_LEN + pld_len);
        req_type = req_type << 1;
        memcpy(ptr+5, &req_type, 1);
        memcpy(ptr+6, &len, 2); 
        memcpy((buffer+HEADER_LEN), pld_with_32bitPadding, pld_len);

    }
    else if(req_type == 0){
        inet_aton(data,&ip_add);
        len = htons(HEADER_LEN + sizeof(ip_add));
        req_type = req_type << 1;
        memcpy(ptr+5, &req_type, 1);
        memcpy(ptr+6, &len, 2); 
        memcpy((buffer+HEADER_LEN), &(ip_add), sizeof(ip_add));
    }
    else if(req_type == 6){
        len = htons(HEADER_LEN);
        req_type = req_type << 1;
        memcpy(ptr+5, &req_type, 1);
        memcpy(ptr+6, &len, 2); 
    }
   
   
    if (sendto(sockfd,buffer, HEADER_LEN + strlen(data),0,(struct sockaddr *)&serv_addr, serv_len) < 0) {
        fprintf(stderr,"ERROR, sending packet from the socket failed");
	return 0;
    }

    if((req_type >> 1) == 6){
        exit(1);
    }
    
    // Function for Timeout
    else {
        fd_set select_fds;                /* fd's used by select */
        struct timeval to;                /* Time value for time out */

        FD_ZERO(&select_fds);             /* Clear out fd's */
        FD_SET(sockfd, &select_fds);      /* Set the interesting fd's */

        to.tv_sec = timeout;		/* Timeout set for 5 sec + 0 micro sec*/
        to.tv_usec = 0;

        if ( select(32, &select_fds, NULL, NULL, &to) == 0 )
        {  
	      printf("Timeout\n");
          return 0;
        }
        
        if( (len = recvfrom(sockfd,buffer_i,255,0,(struct sockaddr *)&serv_addr,&serv_len)) < 0){
                return 0;
        }
    
        int return_type = 0;
        unsigned short ack_num;
        unsigned char *result;// = (char *)malloc(128);
    
        memcpy(&return_type,buffer_i+5,1);
        memcpy(&ack_num,buffer_i+2,2);
        return_type = return_type >> 1;
        if(return_type == 4)
            printf("Unresolved \n");
        else {
            
        // packet is only accepted by the reciever if sequence number is increased by 1
            
            if(ack_num == (seq_no+1)){
                result = buffer_i + HEADER_LEN;
                strtok((char *)result,"\0");
	            printf("%s\n", result);
            }
    }
    }
    return 0;	
}


