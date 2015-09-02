#include "common-client.h"
#include "newport.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <float.h>
#include <math.h>
#include <sstream>
#include <string>

using namespace std;

template <typename T>
string NumberToString(T pNumber)
{
 ostringstream oOStrStream;
 oOStrStream << pNumber;
 return oOStrStream.str();
}


// Multi-thread shared struct
struct cShared{
	LossyReceivingPort *my_recv_port;
	mySendingPort *my_send_port;
	int content_table[256]; //can be changed to char*
	int hostID_int;
};

//Response thread
void *HostResponseProc(void *arg){
	cout << "Starting the file response thread\n" << endl;
	struct cShared *sh = (struct cShared *)arg;
	Packet *recvPacket;
	PacketHdr *hdr;
	int contentid;
	char type;
	int hostid;
	while(1){
		//Response file content
		recvPacket = sh->my_recv_port->receivePacket();
		if (recvPacket != NULL){
			hdr = recvPacket->accessHeader();
			type = hdr->getOctet(0);
			//cout << "Received file type " << (int)type << endl; //testing what type of packet you are dealing with
			if (type == (char)0x00){
				cout << "----You have to response a file! " <<endl;
				contentid = hdr->getIntegerInfo(1);
				hostid = hdr->getIntegerInfo(5);
				if(sh->content_table[contentid]==0){ //check whether there is such a file.
					cout<<"----You don't have the required file to response!"<<endl;
				}
				else{
					std::string filename = NumberToString(contentid);
					filename = filename + ".dat";
					const char *filename_char = filename.c_str();
					ifstream file1(filename_char);
					file1>>noskipws;
					//while(!file1.eof()){
						Packet* cont_packet = new Packet();
						cont_packet->setPayloadSize(1500);
						char* hp=new char[1500];
						for(int j=0;j<1500;j++) //get the file content and fill in the payload
						{
							file1>>hp[j];
						}
						cont_packet->fillPayload(1500,hp);
						PacketHdr *hdr_s = cont_packet->accessHeader(); //configure the header information
						hdr_s->setOctet((char)0x01,0);
						hdr_s->setIntegerInfo(contentid,1);
						hdr_s->setIntegerInfo(hostid,5);
						// hdr_s->setIntegerInfo(sizeof(file1),3); //maybe size of payload is not needed
						sh->my_send_port->sendPacket(cont_packet);
						sh->my_send_port->timer_.stopTimer();
						//sh->my_send_port->lastPkt_ = cont_packet; //try
						//sh->my_send_port->setACKflag(true);
						delete cont_packet;
						// delete hdr_s; //delete the header
					//}
					cout << "----File responsed! " <<endl;
					file1.close();
				}
			}
		}
		//delete recvPacket;
	} //end of while
	return NULL;
}

//update thread
void *HostUpdateProc(void *arg){
	struct cShared *sh = (struct cShared *)arg;
	while(1){
		for (int i=0; i<256; i++){
			if (sh->content_table[i]==1){
				Packet *update_packet = new Packet();
				PacketHdr *hdr_s = update_packet->accessHeader(); //configure the header information
				hdr_s->setOctet((char)0x02,0); //type
				hdr_s->setIntegerInfo(i,1); //contentid
				hdr_s->setIntegerInfo(1,5); //number of hops
				sh->my_send_port->sendPacket(update_packet); //send the packet
				//sh->my_send_port->lastPkt_ = update_packet; //try
				//sh->my_send_port->setACKflag(true);
				cout<<"----you have updated the content "<<i<<endl;
				delete update_packet;
			}
		}
		sleep(10); //sleep for 10 seconds everytime
	}
	return NULL;
}

void *HostRequestProc(void *arg){
	struct cShared *sh = (struct cShared *)arg;
	while(1){
		int mark;
		do{
			cout<<"--------------------------------"<<endl<<"Type the number if you want to "<<endl;
			cout<<"----1.Add a new file to the host"<<endl;
			cout<<"----2.Request a file"<<endl;
			cout<<"----3.Display the files you have."<<endl;
			cout<<"----4.Delete a file from the host"<<endl;
			cout<<"Your choice:"<<endl;
			cin>>mark;
		}while(mark!=1&&mark!=2&&mark!=3&&mark!=4);

		//Case 2: Request a new file.
		if(mark==2){
			int contentid;
			cout<<"Enter the content ID of request file (int 0-255):" << endl;
			cin>>contentid;
			//Send the request file
			if (sh->content_table[contentid]==1){
				cout<<"----You have the file already."<<endl;
			}else{
				Packet * rq_packet = new Packet();
				//rq_packet->setPayloadSize(10);
				PacketHdr *hdr_s = rq_packet->accessHeader(); //configure the header information
				hdr_s->setOctet((char)0x00,0); //type
				hdr_s->setIntegerInfo(contentid,1); //contentid
				hdr_s->setIntegerInfo(sh->hostID_int,5); //hostid, need to be finalize
				sh->my_send_port->sendPacket(rq_packet); //send the packet
				sh->my_send_port->lastPkt_ = rq_packet; //for retransmission
				cout << "----request for content " << contentid << " is sent!" <<endl;
				sh->my_send_port->setACKflag(false);
				sh->my_send_port->timer_.startTimer(2.5); //timer for retransmission, can be modified
				cout << "----begin waiting for content " << contentid << endl;
				Packet *pAck;
				while (!sh->my_send_port->isACKed()){
					pAck = sh->my_recv_port->receivePacket();
					if (pAck!= NULL){
						cout<<"----Response a content file!"<<endl;
						PacketHdr *hdr_check = pAck->accessHeader();
						char type = hdr_check->getOctet(0);
						int contentid_ack = hdr_check->getIntegerInfo(1);
						cout << (int)type << endl;
						if (type == (char)0x01 && contentid_ack == contentid){
							sh->my_send_port->timer_.stopTimer();
							sh->my_send_port->setACKflag(true);
							std::string filename = NumberToString(contentid);
							filename = filename + ".dat";
							const char *filename_char = filename.c_str();
							ofstream file(filename_char,ios::app);
							file<<pAck->getPayload();
							//added by Yao at 09/01/15--start
							sh->content_table[contentid]=1;
							//added by Yao at 09/01/15--end
							file.close();
							cout << "----The requested content " << contentid << " has been received." <<endl;
							//delete pAck;
						}
					}
				}
				delete rq_packet;
			}
		}

		//Case 3: List all the file you have
		if(mark==3){
			for(int i=0; i<256; i++){
				if(sh->content_table[i]==1){
					cout << "----You have content NO. " << i << endl;
				}
			}
		}

		//Case 1: Add a new file to the host content table
		if(mark==1){
			int contentid;
			cout<<"Enter the content ID of adding file (int 0-255):"<<endl;
			cin>>contentid;
			std::string filename_str = NumberToString(contentid);
			filename_str = filename_str + ".dat";
			const char *filename = filename_str.c_str();
			//cout<<"Enter the content file name (should be consistent with the content ID):"<<endl;
			//cin>>filename;
			ifstream file(filename);
			if(file){sh->content_table[contentid]=1;cout<<"----You have added this file!"<<endl;}
			else {cout<< "File not found!"<<endl;}
			file.close();
		};

		//Case 4: Delete a new file to the host content table // only delete the entry in the content table
		if(mark==4){
			int contentid;
			cout<<"Enter the content ID of delete file (int 0-255):"<<endl;
			cin>>contentid;
			if(sh->content_table[contentid]==1){sh->content_table[contentid]=0;cout<< "----You have deleted this file."<<endl;}
			else {cout<< "----You don't have the file."<<endl;}
		}
	}
	return NULL;
}


// Start host

void startHost(){

	printf("----Start client. \n");

	Address *my_rx_addr;
	Address *my_tx_addr;
	Address *dst_addr;
	int dst_addr_int;
	int my_rx_addr_int; //default rx->2
	int my_tx_addr_int;	//default tx->1
	int hostID_int;
	int routerID_int;

	mySendingPort *my_tx_port = new mySendingPort();
	LossyReceivingPort *my_rx_port = new LossyReceivingPort(0.1);
	int cont_table[256];

//	try{

/*		string hname;
		string routername;

		cout << "You are adding a host." << endl;
		cout << "Please enter host ID (int number 0-255): ";
		cin >> hname;
		char* hostID=new char[hname.size()+1]; //convert the string to char* for the host id
		std::copy(hname.begin(),hname.end(),hostID);
		hostID[hname.size()]='\0';
		cout << hostID;

		cout << "Please enter connecting router interface(int number 0-255): ";
		cin >> routername;
		char* routerID = new char[routername.size()+1]; //convert the string to char* for the connecting router id
		std::copy(routername.begin(), routername.end(), routerID);
		routerID[routername.size()] = '\0';
		cout << routerID;
*/
//		char* hostID = "localhost"; //DEFAULT
//		char* routerID = "localhost";

		cout << "Please enter the hostID(int number 0-255):" <<endl;
		cin >> hostID_int;
		my_tx_addr_int = hostID_int+10000;
		//cout << my_tx_addr_int <<endl;
		my_rx_addr_int = hostID_int+20000;
		//cout << my_rx_addr_int <<endl;
		cout << "Please enter the connecting routerID(int number 0-255):" <<endl;
		cin >> routerID_int;
		dst_addr_int = routerID_int+20000;
		//cout << dst_addr_int <<endl;

/*		cout << "Please enter the receiving port addr(int number 0-255): ";
		cin >> my_rx_addr_int;
		cout << "Please enter the sending port addr(int number 0-255): ";
		cin >> my_tx_addr_int;
		cout << "Please enter connecting router recevining port(int number 0-255): ";
		cin >> dst_addr_int;
*/
		my_tx_addr = new Address("localhost", my_tx_addr_int);
		my_rx_addr = new Address("localhost", my_rx_addr_int);
		dst_addr =  new Address("localhost", dst_addr_int);


		//Configure the transmitting port
		my_tx_port->setAddress(my_tx_addr);
		my_tx_port->setRemoteAddress(dst_addr);
		my_tx_port->init();

		//Configure the receiving port
		my_rx_port->setAddress(my_rx_addr);
		my_rx_port->init();

		//Init the content table
		for (int i=0;i<256;i++){cont_table[i]=0;}

/*	}catch(const char *reason ){
		cerr << "Exception:" << reason << endl;
		return;
	}
*/
	printf("----The host has been initialized. \n");

	struct cShared *sh;
	sh = (struct cShared*)malloc(sizeof(struct cShared));
	sh->my_recv_port = my_rx_port;
	sh->my_send_port = my_tx_port;
	sh->content_table[256] = cont_table[256];
	sh->hostID_int = hostID_int;

	/*----Start all the thread.----*/

	pthread_t thread1;
	pthread_create(&(thread1), 0, &HostResponseProc, sh);

	pthread_t thread2;
	pthread_create(&(thread2), 0, &HostUpdateProc, sh);

	pthread_t thread3;
	pthread_create(&(thread3), 0, &HostRequestProc, sh);


	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);

}

// Main function

int main(int argc, const char * argv[]) //input addr format: (1)dest addr name  (2)dest addr  (3)my_rx addr  (4)my_tx addr
{
	startHost();
	return EXIT_SUCCESS;
}
