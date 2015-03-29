#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// Function to add padding to 32 bit boundary
char * 
payload_padding(char *pld, int pld_len)
{

    char *temp_pld = pld;
    int bits_to_pad = 32 - (pld_len % 32); 
    int j;
    for (j=0; j<bits_to_pad; j++) {
        strcat((temp_pld+pld_len+j),"\0");
    } 
    return temp_pld;
}

int main(int argc, char *argv[])
{

    int portno, sockfd, len;
    char *map_file_name, *redirect_file_name;
    char *result;
    char buffer[256];
    char buffer_o[256];
    bzero(buffer,256);
    bzero(buffer_o,256);
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen; 
    FILE *fp1, *fp2;
    int i;
    if (argc < 7) {
        fprintf(stderr,"ERROR, Arguments missing in the executable");
        exit(1);
    }

// Storing arguments from console to variables
    for (i=0; i<argc; i++) {
        if (strcmp(argv[i],"-p") == 0) {
            portno = atoi(argv[i+1]);
	        //printf("portno: %d\n", portno);
            i=i+1;
        }
        else if (strcmp(argv[i],"-f") == 0) {
            map_file_name = argv[i+1];
	        //printf("map_file_name: %s\n", map_file_name);
	        i=i+1;
	   }
	   else if (strcmp(argv[i],"-r") == 0){
	        redirect_file_name = argv[i+1];
	        //printf("redirect_file_name: %s\n",redirect_file_name);
	        i=i+1;
	   }
    }   
    
    // Socket creation

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr,"ERROR, Error opening socket\n");
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))	< 0) {
	   fprintf(stderr,"ERROR, Error while binding socket\n");
    }

    clilen = sizeof(cli_addr); 
    while(1) {
//Creating 2 bufferes one for recieving input packet and other for sending packet
	bzero(buffer, 256);
	bzero(buffer_o, 256);
    if( (len = recvfrom(sockfd,buffer,255,0,(struct sockaddr *)&cli_addr,&clilen)) < 0)
	    printf("Error reading from socket\n");

	unsigned short protno,seq_no,unused;
	char type;
	char *data;
    char *ip_add_str;
    struct in_addr ip_add;
    // Coping entries from buffer to various fields in accordance with their offset and size
    
	memcpy(&protno, buffer, 2);
	//printf("protno: %02x\n", protno);

    memcpy(&seq_no, buffer+2, 2);
	//printf("seq_no: %02x\n", seq_no);

    memcpy(&unused,buffer+4,1);
    memcpy(&type, buffer+5, 1);
	type = type >> 1; //right shift by 1 bit to remove the reserved bit
	//printf("type: %d\n", type); 	        

	data = buffer+8;

    if(type == 1){
        strtok((char *)data,"\0");
	  //  printf("name: %s\n", data);
    }
    else if(type == 0){
        memcpy(&ip_add, buffer+8, sizeof(ip_add));
     //   printf("\n IP Addr ");
	    char *ip_add_str = inet_ntoa(ip_add);
       // printf("\n IP Addr : %s",ip_add_str);
    }

    int return_type;
    char *ptr = buffer_o;

    //protocol: 640 in hex is 0x280
    unsigned short prot = htons(prot);
    memcpy(ptr, &prot, 2);  
    //sequence number
    //printf("\n Seq_num : %d \n",seq_no);
    seq_no+=1;
    memcpy(ptr+2, &seq_no, 2);

    memcpy(ptr+4, &unused, 1);
 
    char buf[128];
    char *result = (char *)malloc(128);
    char *str1 = (char *)malloc(128);
    char *str2 = (char *)malloc(128);
    char *str3 = (char *)malloc(128);
    
    int found = 0;
    
    switch(type){ 
    // Searching for IP address corresponding to the input hostname in map file
    case 1 : //hostname to IP address
        //printf("\n\t Hostname to IP Address Request\n");
        strtok((char *)data,"\0");
	    //printf("name: %s\n", data);
        return_type = 3;
        found = 0;
        fp1 = fopen(map_file_name,"r");
        fgets(buf,128,fp1);
        while(fgets(buf, 128, fp1) != NULL) {
            str1 = strtok(buf," ");
            str2 = strtok(NULL," ");
            if(strcmp(str1,(char *)data) == 0){
                result = str2;
                //printf("\n Match : %s\n",result);
                found = 1;
                break;
            }
        }
        fclose(fp1);
        if(found == 0){
            //printf("\n Entry not found in map file..searching in redirect file \n");
            // Searching for IP address corresponding to the input hostname in redirect file
            fp2 = fopen(redirect_file_name,"r");
            fgets(buf,128,fp2);
            while(fgets(buf, 128, fp2) != NULL) {
                str1 = strtok(buf," ");
                str2 = strtok(NULL," ");
                str3 = strtok(NULL," ");
                //printf("\n Str3 : %s\n",str3);
                if(strcmp(str1,(char *)data) == 0){
                    strcpy(result,str2);
                    strcat(result," ");
                    strcat(result,str3);
                    return_type = 5;
                    found = 1;
                    break;
                }
            }
            fclose(fp2);
        }
        if(found == 0){
            return_type = 4;
        }
        break;
    
    case 0:
        //printf("\n\t IP %s to hostname resolution request\n",data);
         // Searching for hostname corresponding to the input IP address in map file
        memcpy(&ip_add, buffer+8, sizeof(ip_add));
        //printf("\n IP Addr ");
	    data = inet_ntoa(ip_add);
       // printf("\n IP Addr : %s",data);

        return_type = 2;
        found = 0;
        fp1 = fopen(map_file_name,"r");
        fgets(buf,128,fp1);
        while(fgets(buf, 128, fp1) != NULL) {
            str1 = strtok(buf," ");
            str2 = strtok(NULL," ");
            //printf("\n data :%s str1 : %s and str2 : %s\n",data,str1,str2);
            if(strcmp(str2,(char *)data) == 0){
                result = str1;
                //printf("\n Match : %s\n",result);
                found = 1;
                break;
            }
        }
        fclose(fp1);
        if(found == 0){
            //printf("\n Entry not found in map file..searching in redirect file \n");
        // Searching for hostname corresponding to the input IP address in redirect file
            fp2 = fopen(redirect_file_name,"r");
            fgets(buf,128,fp2);
            while(fgets(buf, 128, fp2) != NULL) {
                str1 = strtok(buf," ");
                str2 = strtok(NULL," ");
                str3 = strtok(NULL," ");
                if(strcmp(str1,(char *)data) == 0){
                    strcpy(result,str2);
                    strcat(result," ");
                    strcat(result,str3);
                    return_type = 5;
                    found = 1;
                    break;
                }
            }
            fclose(fp2);
        }
        if(found == 0){
            //printf("\n Unresolved\n");
            return_type = 4;
        }
        
        break;
    case 6:
        printf("\n Terminating Server .... \n");
        close(sockfd);
        return 0;
    //    break;
    default: 
        return_type = 4;
        //printf("\n Error : Invalid request type from client\n");
    }
    
        // Forming a packet to be send to client
    
    return_type = return_type << 1;
    //printf("\n Return Type : %d",return_type);
    memcpy(ptr+5, &return_type, 1);
    
    //printf("\n result = %s\n",result);
    if(found == 1){
        len = 8 + strlen(result);
        memcpy((ptr+8), (payload_padding(result, strlen(result))), strlen(result));
    }
    else{
        len = 8;
    }
    memcpy(ptr+6, &len, 2); 
   

    sleep(3); 
    if (sendto(sockfd,buffer_o, len,0,(struct sockaddr *)&cli_addr, clilen) < 0) {
        fprintf(stderr,"ERROR, sending packet from the socket failed");
	return 0;
    }


    }
     return 0; 
}
