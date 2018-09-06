/* Creates a datagram server.  The port 
   number is passed as an argument.  This
   server runs forever */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

/*for fd*/
#include <fcntl.h>

using namespace std;
void error(const char*);

class myserver {
private:
  int m_sock, length;
  struct sockaddr_in server;

public:
  enum DataType {
  RawData     = 0,
  XmlConfig   = 1,
  XmlStatus   = 2,
  XmlRunStart = 3,
  XmlRunStop  = 4,
  XmlRunTime  = 5
  };

  myserver(int32_t port);

  int getSock() {return m_sock;};
  void listen();
};

myserver::myserver(int32_t port){
  // this will port from 'port' opened on the *localhost* server
  m_sock=socket(AF_INET, SOCK_DGRAM, 0);
  if (m_sock < 0) error("Opening socket");
  length = sizeof(server);
  bzero(&server,length);
  server.sin_family=AF_INET;
  server.sin_addr.s_addr=INADDR_ANY;
  server.sin_port=htons(port);
  if (bind(m_sock,(struct sockaddr *)&server,length)<0) 
    error("binding");
}

void myserver::listen(){
  /*NAIVELY printout what you get from ur client all in char*/
  //int buf_counter;
  char buf[90744];
  
  socklen_t fromlen;
  struct sockaddr_in from;
  fromlen = sizeof(struct sockaddr_in);
  
  while(1){
    puts("[recvfrom] header or other info case");
    int buf_counter = recvfrom(m_sock, buf, 90744,0,
			   (struct sockaddr *)&from, &fromlen);
    if (buf_counter < 0){
      error("recvfrom");
      break;
    }
    
    write(1,"Received a datagram: ",21);
    write(1,buf,buf_counter);
    printf("\n\t==> How long I am? %d\n", buf_counter);
  }
    
}

//--------------------------------------------------------------------------------//
int main(int argc, char *argv[]){
  
  // port given from argv[1]:
  if (argc < 2) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(0);
  } 
  myserver mysr(atoi(argv[1]));
  int sock = mysr.getSock();
  //mysr.listen();
  //return(0);
  
  int buf_counter;
  int sizeReComBuf=0;
  
  socklen_t fromlen;
  struct sockaddr_in from;
  fromlen = sizeof(struct sockaddr_in);
  
  /*output file descriptor*/
  int32_t dataFileFd_=-1;
  string kpixfile("./data/kpixNetData.bin");
  dataFileFd_ = ::open(kpixfile.c_str(),O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  
  if (dataFileFd_<0){
    error("Error -> output bin file corrupted, check!");
    return (1);
  }
  
  bool getaheader=false;
  bool buflost = false;
  uint32_t datasize=0;
  uint32_t datatype=6;
  uint32_t maxbufsize=1024; // not implemented in code currently
  uint32_t *databuf;
  char* charbuf;
  
  while (1) {
    if (getaheader && datasize!=0){
      puts("[recvfrom] xml/rawdata case");
      
      switch (datatype) {
      case myserver::RawData :
	puts("\t[rawdata]\n");
	
	databuf = new uint32_t[datasize*4];
	buf_counter = recvfrom(sock, databuf, datasize*4, 0,
			       (struct sockaddr *)&from,&fromlen);
	if (buf_counter!=datasize*4){
	  printf("[Error] Hmm... buf_counter (%d) != datasize*4 (%d)\n", buf_counter, datasize*4);
	  if (buf_counter==4){
	    buflost = true;
	    cout<< "\n\t [WARN] ==> I found missing oversized data! <=="<<endl;
	    break;
	  }
	}
	write(1,"KPiX RX: ",9);
	write(1,databuf,buf_counter);
	write(dataFileFd_, databuf, buf_counter);
	sizeReComBuf+=buf_counter;
	break;
      case myserver::XmlConfig:   
      case myserver::XmlStatus:      
      case myserver::XmlRunStart:  
      case myserver::XmlRunStop:	   
      case myserver::XmlRunTime:
	puts("\t[xml]\n");
	charbuf = new char[datasize];
	buf_counter = recvfrom(sock, charbuf, datasize, 0,
			       (struct sockaddr *)&from,&fromlen);
	if (buf_counter!=datasize){
	  printf("[Error] Hmm... buf_counter (%d) != xmlsize (%d)\n\t further check data type => %d\n",
		 buf_counter, datasize, datatype);
	  if (buf_counter==4) {
	    buflost = true;
	    cout<< "\n\t [WARN] ==> I found missing oversized data! <=="<<endl;
	    break;
	  }
	}
	write(1,"KPiX RX: ",9);
	write(1,charbuf,buf_counter);
	write(dataFileFd_, charbuf, buf_counter);
	sizeReComBuf+=buf_counter;
	break;
      default:
	puts("unknown type");
	break;
      }
      
      //char databuf[datasize];
      // buf_counter = recvfrom(sock, databuf, sizeof(databuf), 0,
      //		      (struct sockaddr *)&from,&fromlen);
      if (buf_counter < 0) error("recvfrom");
      
      printf("\n\t==> How long I am? %d\n", buf_counter);
      // datasize = 0;
      
    }else { 
      charbuf = new char[1024];
      puts("[recvfrom] header/tail or other cases");
      buf_counter = recvfrom(sock, charbuf, 1024,0,
			     (struct sockaddr *)&from,&fromlen);
      if (buf_counter < 0) error("recvfrom");
      
      write(1,"Received a datagram: ",21);
      write(1,charbuf,buf_counter);
      printf("\n\t==> How long I am? %d\n", buf_counter);
      
      if (buf_counter!=4) getaheader = false; // safety check in case any other information parsed
    }
    
    /*n = sendto(sock,"Got your message\n",17,
      0,(struct sockaddr *)&from,fromlen);
      if (n  < 0) error("sendto");
    */
    
    /*if i found a header/tail w/ fixed length of 4*/
    if ( buf_counter==4 ) {
      uint32_t size;
      std::memcpy(&size, charbuf, sizeof size);

      if (buflost) buflost = false;
            
      if (size == 0xFFFFFFFF) {
	cout<<"\n\t [Info] I found a buffer tail! => Next wait for a header!"<<endl;
	int size_exp=0;
	if (datatype==0) {cout<< "[rawData]";size_exp=datasize*4;}
	else if ( datatype<6 && datatype >0) {cout<<"[xmlData]";size_exp = datasize;}
	else cout<<"@0@ buggy!"<<endl;
	cout <<" => Cumulated size => "<< sizeReComBuf
	     << "; (expected) size => "<< size_exp <<endl;
	
	getaheader = false;
	sizeReComBuf=0;// recomb buffer size set to 0;
	datatype =6;
	datasize =0;
	
      }else{
	cout<<"\n\t [Info] I found a header! => Next wait for a data!"<<endl;
	getaheader=true;
	datasize = (size & 0x0FFFFFFF);
	datatype = (size>>28) & 0xF;
	
	
	cout<<"\n Size: "<< std::hex << size  <<endl;
	cout<<"\n dataSize: "<< std::hex << datasize <<"\n dataType: "
	    << std::dec<< datatype <<endl;
	
	if (datatype>=0 && datatype<6) write(dataFileFd_, &size, 4);  /*xml/raw data*/	  
	else  puts("[WARN] TOCHECK: header not for any data to write!");  /*other data*/	    
      }
      
    }// finished header/tail  
  }// end of while loop

  ::close(dataFileFd_);
  close(sock);
  return 0;
}



void error(const char *msg){
  perror(msg);
  exit(0);
}
