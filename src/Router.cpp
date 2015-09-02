/*
 * Router.cpp
 *
 *  Created on: Apr 28, 2014
 *      Author: administrator
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <float.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include "common.h"

#define TTL 25;
#define rTTL 120;

using namespace std;


struct routing_Table{
	int CID;
	int interf;
	int hopNum;
	int TtoE;
	routing_Table *nextrow;
	routing_Table *previous;

};

struct pending_Table{
	int rCID;
	int HID;
	int interf;
	int TtoE;
	pending_Table *nextrow;
	pending_Table *previous;
};

//shared resource to all thread

struct cShared{
	ReceivingPort* recv_port1;
	ReceivingPort* recv_port2;
	ReceivingPort* recv_port3;
	ReceivingPort* recv_port4;
	SendingPort* send_port1;
	SendingPort* send_port2;
	SendingPort* send_port3;
	SendingPort* send_port4;
	routing_Table* rtable;
	pending_Table* ptable;
	//int flag=0;
};

/* In our design, each router is assigned four interfaces, each of them has its own address (including sending port and receiving port)*/

//Interface 1

void *interface1(void *arg){
	printf("Starting interface1 thread\n");
	struct cShared *sh = (struct cShared *)arg;
	Packet *recvPacket;
	PacketHdr *hdr;

	char type;
	int cid;
	int hid;
	int hopnum;
	int interface;
	while(1){
		recvPacket = sh->recv_port1->receivePacket();
		if(recvPacket!=NULL){

			cout<<"This is interface1!"<<endl;
			sleep(0.1);
		hdr = recvPacket->accessHeader();
		//sh->flag=1;
		type = hdr->getOctet(0);
		//receive update packet: [type+cid+#hops]
		if(type==(char)0x02){
			int k=0;

			struct routing_Table *temp=new routing_Table;
			struct routing_Table *ptr=NULL;
			cid=hdr->getIntegerInfo(1);
			hopnum=hdr->getIntegerInfo(5);
			temp->CID=cid;
			temp->interf=1;
			temp->hopNum=hopnum;
			temp->nextrow=NULL;
			temp->previous=NULL;
			time_t t=time(0);
			temp->TtoE=t+TTL;
			ptr=sh->rtable;
			//update the routing table
            while(ptr!=NULL){
					if(ptr->CID==temp->CID){
						k=1;
						if(ptr->hopNum > temp->hopNum){

							ptr->interf=1;
							ptr->hopNum=temp->hopNum;
							ptr->TtoE=temp->TtoE;
						}
						else if(ptr->hopNum==temp->hopNum&&ptr->interf==temp->interf){
							ptr->TtoE=temp->TtoE;
						}

					}

						ptr=ptr->nextrow;
				}
				if(k==0){
					temp->nextrow=sh->rtable;
					sh->rtable->previous=temp;
					sh->rtable=temp;
				}


			//broadcast updated packet
			hdr->setIntegerInfo(hopnum+1,5);
			sh->send_port2->sendPacket(recvPacket);
			sh->send_port3->sendPacket(recvPacket);
			sh->send_port4->sendPacket(recvPacket);
			//delete temp;

		}
		//receive response packet: [type+cid+hid+payload]
		else if(type==(char)0x01){
			int k=0;

			struct pending_Table *ptr=NULL;
			ptr=sh->ptable;
			cid=hdr->getIntegerInfo(1);
			hid=hdr->getIntegerInfo(5);
			//find the path in pending table
			while(ptr!=NULL){
				if(ptr->rCID==cid && ptr->HID==hid){
					interface=ptr->interf;
					k=1;
					//decided which interface should send the packet
					switch(interface){
						case 1:
							sh->send_port1->sendPacket(recvPacket);
							break;
						case 2:
							sh->send_port2->sendPacket(recvPacket);
							break;
						case 3:
							sh->send_port3->sendPacket(recvPacket);
							break;
						case 4:
							sh->send_port4->sendPacket(recvPacket);
							break;
						default:
							cout<<"Cannot found path!"<<endl;
							break;
					}
				}
				if(k==1)
					break;
			}
			//cout<<"Delivery Error!"<<endl;
			//delete ptr;
		}

		//receive request packet: [type+cid+hid]
		else{
			int k=0;
			int k1=0;

			struct pending_Table *ptemp=new pending_Table;
			struct pending_Table *pptr=NULL;
			struct routing_Table *rptr=NULL;

			cid=hdr->getIntegerInfo(1);
			hid=hdr->getIntegerInfo(5);



			pptr=sh->ptable;
			rptr=sh->rtable;

			ptemp->rCID=cid;
			ptemp->HID=hid;
			ptemp->interf=1;
			time_t t=time(0);
			ptemp->TtoE=t+rTTL;
			ptemp->nextrow=NULL;
			ptemp->previous=NULL;


			//update the pending table
			while(pptr!=NULL){
				if(pptr->rCID==ptemp->rCID && pptr->HID==ptemp->HID){
					k=1;
					//reset TtoE
					break;
				}
				pptr=pptr->nextrow;
			}
			if(k==0){
				ptemp->nextrow=sh->ptable;
				sh->ptable->previous=ptemp;
				sh->ptable=ptemp;
			}
			//figure the path out with routing table

			while(rptr!=NULL){
				if(rptr->CID==ptemp->rCID){
					k1=1;
					interface=rptr->interf;

					//decided which interface should send the packet
					switch(interface){
						case 1:
							sh->send_port1->sendPacket(recvPacket);
							break;
						case 2:
							sh->send_port2->sendPacket(recvPacket);
							break;
						case 3:
							sh->send_port3->sendPacket(recvPacket);
							break;
						case 4:
							sh->send_port4->sendPacket(recvPacket);
							break;
						default:
							cout<<"Cannot found path!"<<endl;
							break;
					}
				}
				if(k1==1)
					break;
				rptr=rptr->nextrow;
			}
			if(k1==0)
				cout<<"Cannot find such file!"<<endl;
			//cout<<"Cannot found such content!"<<endl;
		}

		struct routing_Table *p=NULL;
		p=sh->rtable;
		cout<<"The Routing Table is:"<<endl;
		cout<<"ContentID"<<setw(10)<<"Interface"<<setw(6)<<"#Hops"<<setw(5)<<"TtoE"<<endl;
		while(p->nextrow!=NULL){
			cout<<setw(7)<<p->CID<<setw(10)<<p->interf;
			cout<<setw(5)<<p->hopNum<<setw(7)<<p->TtoE-time(0)<<endl;
			p=p->nextrow;
		}

		struct pending_Table *t=NULL;
		t=sh->ptable;
		cout<<"The Pending Table is:"<<endl;
		cout<<"RequstCID"<<setw(7)<<"HostID"<<setw(10)<<"Interface"<<setw(10)<<"TimeToExp"<<endl;
		while(t->nextrow!=NULL){
			cout<<setw(7)<<t->rCID<<setw(7)<<t->HID;
			cout<<setw(9)<<t->interf<<setw(10)<<t->TtoE-time(0)<<endl;
			t=t->nextrow;
		}
	}

	}
	return NULL;

}

//Interface-2

void *interface2(void *arg){
	printf("Starting interface2 thread\n");
		struct cShared *sh = (struct cShared *)arg;
		Packet *recvPacket;
		PacketHdr *hdr;

		char type;
		int cid;
		int hid;
		int hopnum;
		int interface;
		while(1){
			sleep(0.2);
			recvPacket = sh->recv_port2->receivePacket();
			if(recvPacket!=NULL){

			cout<<"This is interface2!"<<endl;
			hdr = recvPacket->accessHeader();
			type = hdr->getOctet(0);

			//receive update packet: [type+cid+#hops]
			if(type==(char)0x02){
				int k=0;

				struct routing_Table *temp=new routing_Table;
				struct routing_Table *ptr=NULL;
				cid=hdr->getIntegerInfo(1);
				hopnum=hdr->getIntegerInfo(5);
				temp->CID=cid;
				temp->interf=2;
				temp->hopNum=hopnum;
				temp->nextrow=NULL;
				temp->previous=NULL;
				time_t t=time(0);
				temp->TtoE=t+TTL;

				ptr=sh->rtable;

				//update the routing table
					while(ptr!=NULL){
						if(ptr->CID==temp->CID){
							k=1;
							if(ptr->hopNum > temp->hopNum){

								ptr->interf=2;
								ptr->hopNum=temp->hopNum;
								ptr->TtoE=temp->TtoE;
							}
							else if(ptr->hopNum==temp->hopNum && ptr->interf==temp->interf){
								ptr->TtoE=temp->TtoE;
							}

						}
							ptr=ptr->nextrow;
					}
					if(k==0){
						temp->nextrow=sh->rtable;
						sh->rtable->previous=temp;
						sh->rtable=temp;
					}

				//broadcast updated packet
				hdr->setIntegerInfo(hopnum+1,5);
				sh->send_port1->sendPacket(recvPacket);
				sh->send_port3->sendPacket(recvPacket);
				sh->send_port4->sendPacket(recvPacket);
				//delete temp;

			}
			//receive response packet: [type+cid+hid+payload]
			else if(type==(char)0x01){
				int k=0;
				struct pending_Table *ptr=NULL;
				ptr=sh->ptable;
				cid=hdr->getIntegerInfo(1);
				hid=hdr->getIntegerInfo(5);
				//find the path in pending table

                while(ptr!=NULL){
					if(ptr->rCID==cid && ptr->HID==hid){
						interface=ptr->interf;
						k=1;
						//decided which interface should send the packet
						switch(interface){
							case 1:
								sh->send_port1->sendPacket(recvPacket);
								break;
							case 2:
								sh->send_port2->sendPacket(recvPacket);
								break;
							case 3:
								sh->send_port3->sendPacket(recvPacket);
								break;
							case 4:
								sh->send_port4->sendPacket(recvPacket);
								break;
							default:
								cout<<"Cannot found path!"<<endl;
								break;
						}
					}
					if(k==1)
						break;
				}
			}

			//receive request packet: [type+cid+hid]
			else{
				int k=0;
				int k1=0;

				struct pending_Table *ptemp=new pending_Table;
				struct pending_Table *pptr=NULL;
				struct routing_Table *rptr=NULL;

				cid=hdr->getIntegerInfo(1);
				hid=hdr->getIntegerInfo(5);


				pptr=sh->ptable;
				rptr=sh->rtable;

				ptemp->rCID=cid;
				ptemp->HID=hid;
				ptemp->interf=2;
				time_t t=time(0);
				ptemp->TtoE=t+rTTL;
				ptemp->nextrow=NULL;
				ptemp->previous=NULL;

				//update the pending table
				while(pptr!=NULL){
					if(pptr->rCID==ptemp->rCID && pptr->HID==ptemp->HID){
						k=1;
						//reset TtoE
						break;
					}
					pptr=pptr->nextrow;
				}
				if(k==0){
					ptemp->nextrow=sh->ptable;
					sh->ptable->previous=ptemp;
					sh->ptable=ptemp;
				}
				//figure the path out with routing table

				while(rptr!=NULL){
					if(rptr->CID==ptemp->rCID){
						k1=1;
						interface=rptr->interf;

						//decided which interface should send the packet
						switch(interface){
							case 1:
								sh->send_port1->sendPacket(recvPacket);
								break;
							case 2:
								sh->send_port2->sendPacket(recvPacket);
								break;
							case 3:
								sh->send_port3->sendPacket(recvPacket);
								break;
							case 4:
								sh->send_port4->sendPacket(recvPacket);
								break;
							default:
								cout<<"Cannot found path!"<<endl;
								break;
						}
					}
					if(k1==1)
						break;
					rptr=rptr->nextrow;
				}
				if(k1==0)
					cout<<"Cannot find such file!"<<endl;
				//cout<<"Cannot found such content!"<<endl;
				//delete pptr;
				//delete rptr;
				//delete ptemp;
			}
			//delete recvPacket;
			struct routing_Table *p=NULL;
			p=sh->rtable;
			cout<<"The Routing Table is:"<<endl;
			cout<<"ContentID"<<setw(10)<<"Interface"<<setw(6)<<"#Hops"<<setw(5)<<"TtoE"<<endl;
			while(p->nextrow!=NULL){
				cout<<setw(7)<<p->CID<<setw(10)<<p->interf;
				cout<<setw(5)<<p->hopNum<<setw(7)<<p->TtoE-time(0)<<endl;
				p=p->nextrow;
			}

			struct pending_Table *t=NULL;
			t=sh->ptable;
			cout<<"The Pending Table is:"<<endl;
			cout<<"RequstCID"<<setw(7)<<"HostID"<<setw(10)<<"Interface"<<setw(10)<<"TimeToExp"<<endl;
			while(t->nextrow!=NULL){
				cout<<setw(7)<<t->rCID<<setw(7)<<t->HID;
				cout<<setw(9)<<t->interf<<setw(10)<<t->TtoE-time(0)<<endl;
				t=t->nextrow;
			}
		}
		}
		return NULL;
}


//Interface 3

void *interface3(void *arg){
	printf("Starting interface3 thread\n");
	struct cShared *sh = (struct cShared *)arg;
	Packet *recvPacket;
	PacketHdr *hdr;

	char type;
	int cid;
	int hid;
	int hopnum;
	int interface;

	while(1){
		sleep(0.3);
			recvPacket = sh->recv_port3->receivePacket();


			if(recvPacket!=NULL){
				cout<<"This is interface 3!"<<endl;
			hdr = recvPacket->accessHeader();
			type = hdr->getOctet(0);

			//receive update packet: [type+cid+#hops]
			if(type==(char)0X02){
				int k=0;

				struct routing_Table *temp=new routing_Table;
				struct routing_Table *ptr=NULL;
				cid=hdr->getIntegerInfo(1);
				hopnum=hdr->getIntegerInfo(5);
				temp->CID=cid;
				temp->interf=3;
				temp->hopNum=hopnum;
				temp->nextrow=NULL;
				temp->previous=NULL;
				time_t t=time(0);
				temp->TtoE=t+TTL;

				ptr=sh->rtable;
				//update the routing table
					while(ptr!=NULL){
						if(ptr->CID==temp->CID){
							k=1;
							if(ptr->hopNum > temp->hopNum){

								ptr->interf=3;
								ptr->hopNum=temp->hopNum;
								ptr->TtoE=temp->TtoE;
							}
							else if(ptr->hopNum==temp->hopNum&&ptr->interf==temp->interf){
								ptr->TtoE=temp->TtoE;
							}

						}
							ptr=ptr->nextrow;
					}
					if(k==0){
						temp->nextrow=sh->rtable;
						sh->rtable->previous=temp;
						sh->rtable=temp;
					};
				//}

				//broadcast updated packet
				hdr->setIntegerInfo(hopnum+1,5);
				sh->send_port2->sendPacket(recvPacket);
				sh->send_port1->sendPacket(recvPacket);
				sh->send_port4->sendPacket(recvPacket);

			}
			//receive response packet: [type+cid+hid+payload]
			else if(type==(char)0x01){
				int k=0;
				struct pending_Table *ptr=NULL;
				ptr=sh->ptable;
				cid=hdr->getIntegerInfo(1);
				hid=hdr->getIntegerInfo(5);
				//find the path in pending table
				while(ptr!=NULL){
					if(ptr->rCID==cid && ptr->HID==hid){
						interface=ptr->interf;
						k=1;
						//decided which interface should send the packet
						switch(interface){
							case 1:
								sh->send_port1->sendPacket(recvPacket);
								break;
							case 2:
								sh->send_port2->sendPacket(recvPacket);
								break;
							case 3:
								sh->send_port3->sendPacket(recvPacket);
								break;
							case 4:
								sh->send_port4->sendPacket(recvPacket);
								break;
							default:
								cout<<"Cannot found path!"<<endl;
								break;
						}
					}
					if(k==1)
						break;
				}
				//cout<<"Delivery Error!"<<endl;
				//delete ptr;
			}

			//receive request packet: [type+cid+hid]
			else{
				int k=0;
				int k1=0;

				struct pending_Table *ptemp=new pending_Table;
				struct pending_Table *pptr=NULL;
				struct routing_Table *rptr=NULL;

				cid=hdr->getIntegerInfo(1);
				hid=hdr->getIntegerInfo(5);

				pptr=sh->ptable;
				rptr=sh->rtable;

				ptemp->rCID=cid;
				ptemp->HID=hid;
				ptemp->interf=3;
				time_t t=time(0);
				ptemp->TtoE=t+rTTL;
				ptemp->nextrow=NULL;
				ptemp->previous=NULL;


				//update the pending table
				while(pptr!=NULL){
					if(pptr->rCID==ptemp->rCID && pptr->HID==ptemp->HID){
						k=1;
						//reset TtoE
						break;
					}
					pptr=pptr->nextrow;
				}
				if(k==0){
					ptemp->nextrow=sh->ptable;
					sh->ptable->previous=ptemp;
					sh->ptable=ptemp;
				}
				//figure the path out with routing table


				while(rptr!=NULL){
					if(rptr->CID==ptemp->rCID){
						k1=1;
						interface=rptr->interf;

						//decided which interface should send the packet
						switch(interface){
							case 1:
								sh->send_port1->sendPacket(recvPacket);
								break;
							case 2:
								sh->send_port2->sendPacket(recvPacket);
								break;
							case 3:
								sh->send_port3->sendPacket(recvPacket);
								break;
							case 4:
								sh->send_port4->sendPacket(recvPacket);
								break;
							default:
								cout<<"Cannot found path!"<<endl;
								break;
						}
					}
					if(k1==1)
						break;
					rptr=rptr->nextrow;
				}
				if(k1==0)
					cout<<"Cannot find such content!"<<endl;
			}
			//delete recvPacket;

			struct routing_Table *p=NULL;
			p=sh->rtable;
			cout<<"The Routing Table is:"<<endl;
			cout<<"ContentID"<<setw(10)<<"Interface"<<setw(6)<<"#Hops"<<setw(5)<<"TtoE"<<endl;
			while(p->nextrow!=NULL){
				cout<<setw(7)<<p->CID<<setw(10)<<p->interf;
				cout<<setw(5)<<p->hopNum<<setw(7)<<p->TtoE-time(0)<<endl;
				p=p->nextrow;
			}

			struct pending_Table *t=NULL;
			t=sh->ptable;
			cout<<"The Pending Table is:"<<endl;
			cout<<"RequstCID"<<setw(7)<<"HostID"<<setw(10)<<"Interface"<<setw(10)<<"TimeToExp"<<endl;
			while(t->nextrow!=NULL){
				cout<<setw(7)<<t->rCID<<setw(7)<<t->HID;
				cout<<setw(9)<<t->interf<<setw(10)<<t->TtoE-time(0)<<endl;
				t=t->nextrow;
			}
			}

		}
	return NULL;
}

//Interface 4

void *interface4(void *arg){
	printf("Starting interface4 thread\n");
	struct cShared *sh = (struct cShared *)arg;
	cout<<"This is interface4!"<<endl;

	Packet *recvPacket;
	PacketHdr *hdr;

	char type;
	int cid;
	int hid;
	int hopnum;
	int interface;

	while(1){
		sleep(0.4);
		recvPacket = sh->recv_port4->receivePacket();
		if(recvPacket!=NULL){
            
		hdr = recvPacket->accessHeader();
		type = hdr->getOctet(0);


		//receive update packet: [type+cid+#hops]
		if(type==(char)0X02){
			int k=0;

			struct routing_Table *temp=new routing_Table;
			struct routing_Table *ptr=NULL;
			cid=hdr->getIntegerInfo(1);
			hopnum=hdr->getIntegerInfo(5);
			temp->CID=cid;
			temp->interf=4;
			temp->hopNum=hopnum;
			temp->nextrow=NULL;
			temp->previous=NULL;
			time_t t=time(0);
			temp->TtoE=t+TTL;

			ptr=sh->rtable;

			//update the routing table
				while(ptr!=NULL){
					if(ptr->CID==temp->CID){
						k=1;
						if(ptr->hopNum > temp->hopNum){

							ptr->interf=4;
							ptr->hopNum=temp->hopNum;
							ptr->TtoE=temp->TtoE;
						}
						else if(ptr->hopNum==temp->hopNum&&ptr->interf==temp->interf){
							ptr->TtoE=temp->TtoE;
						}

					}
						ptr=ptr->nextrow;
				}
				if(k==0){
					temp->nextrow=sh->rtable;
					sh->rtable->previous=temp;
					sh->rtable=temp;
				}
			//}
			//delete ptr;


			//broadcast updated packet
			hdr->setIntegerInfo(hopnum+1,5);
			sh->send_port2->sendPacket(recvPacket);
			sh->send_port3->sendPacket(recvPacket);
			sh->send_port1->sendPacket(recvPacket);

		}
		//receive response packet: [type+cid+hid+payload]
		else if(type==(char)0x01){
			int k=0;
			struct pending_Table *ptr=NULL;
			ptr=sh->ptable;
			cid=hdr->getIntegerInfo(1);
			hid=hdr->getIntegerInfo(5);
			//find the path in pending table
			while(ptr!=NULL){
				if(ptr->rCID==cid && ptr->HID==hid){
					interface=ptr->interf;
					k=1;
					//decided which interface should send the packet
					switch(interface){
						case 1:
							sh->send_port1->sendPacket(recvPacket);
							break;
						case 2:
							sh->send_port2->sendPacket(recvPacket);
							break;
						case 3:
							sh->send_port3->sendPacket(recvPacket);
							break;
						case 4:
							sh->send_port4->sendPacket(recvPacket);
							break;
						default:
							cout<<"Cannot found path!"<<endl;
							break;
					}
				}
				if(k==1)
					break;
			}
			//cout<<"Delivery Error!"<<endl;
			//delete ptr;
		}

		//receive request packet: [type+cid+hid]
		else{
			int k=0;
			int k1=0;

			struct pending_Table *ptemp=new pending_Table;
			struct pending_Table *pptr=NULL;
			struct routing_Table *rptr=NULL;

			cid=hdr->getIntegerInfo(1);
			hid=hdr->getIntegerInfo(5);


			pptr=sh->ptable;
			rptr=sh->rtable;

			ptemp->rCID=cid;
			ptemp->HID=hid;
			ptemp->interf=4;
			time_t t=time(0);
			ptemp->TtoE=t+rTTL;
			ptemp->nextrow=NULL;
			ptemp->previous=NULL;


			//update the pending table
			while(pptr!=NULL){
				if(pptr->rCID==ptemp->rCID && pptr->HID==ptemp->HID){
					k=1;
					//reset TtoE
					break;
				}
				pptr=pptr->nextrow;
			}
			if(k==0){
				ptemp->nextrow=sh->ptable;
				sh->ptable->previous=ptemp;
				sh->ptable=ptemp;
			}
			//figure the path out with routing table


			while(rptr!=NULL){
				if(rptr->CID==ptemp->rCID){
					k1=1;
					interface=rptr->interf;

					//decided which interface should send the packet
					switch(interface){
						case 1:
							sh->send_port1->sendPacket(recvPacket);
							break;
						case 2:
							sh->send_port2->sendPacket(recvPacket);
							break;
						case 3:
							sh->send_port3->sendPacket(recvPacket);
							break;
						case 4:
							sh->send_port4->sendPacket(recvPacket);
							break;
						default:
							cout<<"Cannot found path!"<<endl;
							break;
					}
				}
				if(k1==1)
					break;
				rptr=rptr->nextrow;
			}
			if(k1==0)
				cout<<"Cannot found such content!"<<endl;
			//delete pptr;
			//delete rptr;
			//delete ptemp;
		}
		//delete recvPacket;

		struct routing_Table *p=NULL;
		p=sh->rtable;
		cout<<"The Routing Table is:"<<endl;
		cout<<"ContentID"<<setw(10)<<"Interface"<<setw(6)<<"#Hops"<<setw(5)<<"TtoE"<<endl;
		while(p->nextrow!=NULL){
			cout<<setw(7)<<p->CID<<setw(10)<<p->interf;
			cout<<setw(5)<<p->hopNum<<setw(7)<<p->TtoE-time(0)<<endl;
			p=p->nextrow;
		}

		struct pending_Table *t=NULL;
		t=sh->ptable;
		cout<<"The Pending Table is:"<<endl;
		cout<<"RequstCID"<<setw(7)<<"HostID"<<setw(10)<<"Interface"<<setw(10)<<"TimeToExp"<<endl;
		while(t->nextrow!=NULL){
			cout<<setw(7)<<t->rCID<<setw(7)<<t->HID;
			cout<<setw(9)<<t->interf<<setw(10)<<t->TtoE-time(0)<<endl;
			t=t->nextrow;
		}
		}
	}
	return NULL;
}
void *timecounter(void *arg){
	printf("Starting TimeCounter thread\n");
	struct cShared *sh = (struct cShared *)arg;

	while(1){
		sleep(1);
		struct routing_Table *p=NULL;
		p=sh->rtable;
		struct pending_Table *t=NULL;
		t=sh->ptable;
		while(p->nextrow!=NULL){
			p->TtoE--;
			if(p->TtoE<time(0)){
				if(p->previous!=NULL&&p->nextrow!=NULL){
					p->previous->nextrow=p->nextrow;
					p->nextrow->previous=p->previous;
				}
				else if(p->previous==NULL&&p->nextrow!=NULL){
					p=p->nextrow;
					p->previous=NULL;
				}
				else{
					p->previous->nextrow=p->nextrow;
				}
			}
			if(p->nextrow!=NULL)
				p=p->nextrow;

		}



		while(t->nextrow!=NULL){
			t->TtoE--;
			if(t->TtoE<time(0)){
				if(t->previous!=NULL&&t->nextrow!=NULL){
					t->previous->nextrow=t->nextrow;
					t->nextrow->previous=t->previous;
				}
				else if(t->previous==NULL&&t->nextrow!=NULL){
					t=t->nextrow;
					t->previous=NULL;
				}
				else{
					t->previous->nextrow=t->nextrow;
				}
			}
			if(t->nextrow!=NULL)
				t=t->nextrow;
		}
	}



	return NULL;
}


// Main function
// The user need to input the interface address and its connecting neighbour address.
// You don't have to input the port number since the port is automatically generated for each node by sendingport=addr+10000 and receivingport=addr+20000


int main(int argc, char *argv[]) {

	struct routing_Table *rT;
	rT = (struct routing_Table*)malloc(sizeof(struct routing_Table));

	rT->CID=0;
	rT->interf=0;
	rT->hopNum=0;
	rT->interf=0;
	rT->TtoE=30;
	rT->nextrow=NULL;

	struct pending_Table *pT;
	pT = (struct pending_Table*)malloc(sizeof(struct pending_Table));

	pT->HID=0;
	pT->interf=0;
	pT->rCID=0;
	pT->TtoE=30;
	pT->nextrow=NULL;


	Address *my_recv_addr1;
	Address *my_send_addr1;
	Address *dst_addr1;
	SendingPort *my_send_port1;
	ReceivingPort *my_recv_port1;

	Address *my_recv_addr2;
	Address *my_send_addr2;
	Address *dst_addr2;
	SendingPort *my_send_port2;
	ReceivingPort *my_recv_port2;

	Address *my_recv_addr3;
	Address *my_send_addr3;
	Address *dst_addr3;
	SendingPort *my_send_port3;
	ReceivingPort *my_recv_port3;

	Address *my_recv_addr4;
	Address *my_send_addr4;
	Address *dst_addr4;
	SendingPort *my_send_port4;
	ReceivingPort *my_recv_port4 ;
	try{


		int interfaceID,destID;
		cout<<"Input interface1 id:";
		cin>>interfaceID;

		cout<<"Input destination ID of interface1:";
		cin>>destID;
		//interfaceID=2;
		//destID=1;

		short int sendingPort=interfaceID+10000;
		short int recvingPort=interfaceID+20000;

		short int destport=destID+20000;


		my_recv_addr1= new Address("localhost",recvingPort);
		my_send_addr1 = new Address("localhost", sendingPort);
		dst_addr1 =  new Address("localhost", destport);

//		cout<<interfaceID<<endl;
//		cout<<my_recv_addr1<<endl;

		my_send_port1 = new SendingPort();
		my_send_port1->setAddress(my_send_addr1);
		my_send_port1->setRemoteAddress(dst_addr1);
		my_send_port1->init();



		my_recv_port1 = new ReceivingPort();
		my_recv_port1->setAddress(my_recv_addr1);
		my_recv_port1->init();


		cout<<"Input the interface2 id:";
		cin>>interfaceID;
		cout<<"Input the destination ID of interface2:";
		cin>>destID;

		//interfaceID=4;
		//destID=6;


		sendingPort=interfaceID+10000;
		recvingPort=interfaceID+20000;
		destport=destID+20000;

		my_recv_addr2 = new Address("localhost", recvingPort);
		my_send_addr2 = new Address("localhost", sendingPort);
		dst_addr2 =  new Address("localhost", destport);
		my_send_port2 = new SendingPort();
		my_send_port2->setAddress(my_send_addr2);
		my_send_port2->setRemoteAddress(dst_addr2);
		my_send_port2->init();
		my_recv_port2 = new ReceivingPort();
		my_recv_port2->setAddress(my_recv_addr2);
		my_recv_port2->init();

		cout<<"Input the interface3 id:";
		cin>>interfaceID;
		cout<<"Input the destination ID of interface3:";
		cin>>destID;

		//interfaceID=4;
		//destID=6;


		sendingPort=interfaceID+10000;
		recvingPort=interfaceID+20000;
		destport=destID+20000;

		my_recv_addr3 = new Address("localhost", recvingPort);
		my_send_addr3 = new Address("localhost", sendingPort);
		dst_addr3 =  new Address("localhost", destport);
		my_send_port3 = new SendingPort();
		my_send_port3->setAddress(my_send_addr3);
		my_send_port3->setRemoteAddress(dst_addr3);
		my_send_port3->init();
		my_recv_port3 = new ReceivingPort();
		my_recv_port3->setAddress(my_recv_addr3);
		my_recv_port3->init();


		cout<<"Input the interface4 id:";
		cin>>interfaceID;
		cout<<"Input the destination ID of interface4:";
		cin>>destID;

		//interfaceID=5;
		//destID=8;

		sendingPort=interfaceID+10000;
		recvingPort=interfaceID+20000;
		destport=destID+20000;

		my_recv_addr4 = new Address("localhost", recvingPort);
		my_send_addr4 = new Address("localhost", sendingPort);
		dst_addr4 =  new Address("localhost", destport);
		my_send_port4 = new SendingPort();
		my_send_port4->setAddress(my_send_addr4);
		my_send_port4->setRemoteAddress(dst_addr4);
		my_send_port4->init();
		my_recv_port4 = new ReceivingPort();
		my_recv_port4->setAddress(my_recv_addr4);
		my_recv_port4->init();

	} catch(const char *reason ){
		cerr << "Exception:" << reason << endl;
		return EXIT_SUCCESS;
	}
	printf("Router start.\n");


	struct cShared *sh;
	sh = (struct cShared*)malloc(sizeof(struct cShared));



		sh->recv_port1 = my_recv_port1;
		sh->send_port1 = my_send_port1;
		sh->recv_port2 = my_recv_port2;
		sh->send_port2 = my_send_port2;
		sh->recv_port3 = my_recv_port3;
		sh->send_port3 = my_send_port3;
		sh->recv_port4 = my_recv_port4;
		sh->send_port4 = my_send_port4;
		sh->rtable=rT;
		sh->ptable=pT;


		pthread_t thread1;
		pthread_create(&(thread1), 0, &interface1, sh);
		pthread_t thread2;
		pthread_create(&(thread2), 0, &interface2, sh);
		pthread_t thread3;
		pthread_create(&(thread3), 0, &interface3, sh);
		pthread_t thread4;
		pthread_create(&(thread4), 0, &interface4, sh);
		pthread_t thread5;
		pthread_create(&(thread5), 0, &timecounter, sh);



		pthread_join(thread1,NULL);
		pthread_join(thread2,NULL);
		pthread_join(thread3,NULL);
		pthread_join(thread4,NULL);
		pthread_join(thread5,NULL);


		/*Packet *recvPacket1;
		PacketHdr *hdr1;
		int seq_n1;
		char type1;
		int size1;
		recvPacket1 = my_recv_port1->receivePacket();
		hdr1 = recvPacket1->accessHeader();
		type1 = hdr1->getOctet(0);
		seq_n1 = hdr1->getIntegerInfo(3);
		size1 = recvPacket1->getPayloadSize();
		printf("Received Bpacket: type = %c, sequence number = %d, size = %d\n", type1, seq_n1, size1);
		//hdr->setOctet('1', 0);
		my_send_port2->sendPacket(recvPacket1);*/

	//startServer();
	return EXIT_SUCCESS;
}


