#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h> // gettimeofday()
#include <signal.h>
// ICMP header len for echo req
#define ICMP_HDRLEN 8

// Checksum algo
unsigned short calculate_checksum(unsigned short * paddress, int len);

//google ip
#define DESTINATION_IP "10.0.2.4"
static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

int main ()
{
    FILE *fd;
    fd=fopen("flood_ping.txt","w+");
    double sum=0;
    int count=0;
    signal (SIGINT,intHandler);
    while(keepRunning){
    count++;
    struct icmp icmphdr; // ICMP-header
    char data[50] = {0};//"AI\n"; //{0};
    char buf[IP_MAXPACKET];
    bzero(buf,IP_MAXPACKET);
    socklen_t sock_addr_len;
    struct sockaddr_in cliaddr;
    struct timeval begin,end;


    int datalen = strlen(data) + 1;//because its a string

    //===================
    // ICMP header
    //===================

    // Message Type (8 bits): ICMP_ECHO_REQUEST
    icmphdr.icmp_type = ICMP_ECHO;

    // Message Code (8 bits): echo request
    icmphdr.icmp_code = 0;

    // Identifier (16 bits): some number to trace the response.
    // It will be copied to the response packet and used to map response to the request sent earlier.
    // Thus, it serves as a Transaction-ID when we need to make "ping"
    icmphdr.icmp_id = 18;

    // Sequence Number (16 bits): starts at 0
    icmphdr.icmp_seq = 0;

    // ICMP header checksum (16 bits): set to 0 not to include into checksum calculation
    icmphdr.icmp_cksum = 0;

    // Combine the packet
    char packet[IP_MAXPACKET];

    // Next, ICMP header
    memcpy ((packet), &icmphdr, ICMP_HDRLEN);

    // After ICMP header, add the ICMP data.
    memcpy (packet+ICMP_HDRLEN, data, datalen);

    // Calculate the ICMP header checksum
    icmphdr.icmp_cksum = calculate_checksum((unsigned short *) (packet), ICMP_HDRLEN + datalen);
    memcpy ((packet ), &icmphdr, ICMP_HDRLEN);

    struct sockaddr_in dest_in;
    memset (&dest_in, 0, sizeof (struct sockaddr_in));
    dest_in.sin_family = AF_INET;

    // The port is irrelevant for Networking and therefore was zeroed.
    dest_in.sin_addr.s_addr = inet_addr(DESTINATION_IP);

    // Create raw socket for icmp
    int sock = -1;
    if ((sock = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        fprintf (stderr, "socket() failed with error: %d", errno);
        fprintf (stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
        return -1;
    }
    gettimeofday(&begin,0);
    // Send the packet using sendto() for sending datagrams.
    if (sendto (sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *) &dest_in, sizeof (dest_in)) == -1)
    {
        fprintf (stderr, "sendto() failed with error: %d", errno);
        return -1;
    }
    int n=0;
    sock_addr_len = sizeof(struct sockaddr_in);
     while(1)
     {
         n=recvfrom(sock,buf,IP_MAXPACKET,0,(struct sockaddr *)&cliaddr,&sock_addr_len);
            if(n<0)
            {
                fprintf(stderr, "recv() failed with error: %d", errno);
                return -1;
            }
            else
         {
              //  printf("the data from the server is: ");
             for (int i = ICMP_HDRLEN+sock_addr_len; i < n; i++) {
                 printf("%c", buf[i]);
             }
             break;
         }
     }
    gettimeofday(&end,0);
    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    double time = seconds+microseconds*1e-6;
    sum+=time;
    printf("RTT time  in ms is: %lf\n",time*1000);
    char buffer[50];
    sprintf(buffer, "%d, %f",count,time*1000);
    fprintf(fd, "%s \n",buffer);
    fflush(fd);
    // Close the raw socket descriptor.
    sleep(5);
    close(sock);
    }
    
    double avg_t=sum/count;
    printf("\n RTT AVG time  in ms is: %lf\n",avg_t*1000);
    char buffer2[100];
    sprintf(buffer2, "\n RTT AVG time  in ms is: %f",avg_t*1000);
    fprintf(fd, "%s \n",buffer2);
    fflush(fd);
   fclose(fd);

    return 0;
}

// Compute checksum (RFC 1071).
unsigned short calculate_checksum(unsigned short * paddress, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short * w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *((unsigned char *)&answer) = *((unsigned char *)w);
        sum += answer;
    }

    // add back carry outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
    sum += (sum >> 16);                 // add carry
    answer = ~sum;                      // truncate to 16 bits

    return answer;
}