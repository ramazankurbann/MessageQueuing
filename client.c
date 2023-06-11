/*
 * client.c: Client program
 *           to demonstrate interprocess commnuication
 *           with System V message queues
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <signal.h>

#include <pthread.h>

const uint32_t ServerSourceID = 1000000;
uint32_t ClientSourceID;
uint32_t OwnInputPortID;
uint32_t OwnOutputPortID;

uint32_t ServerPortID;

const enum MessageIDs
{
	ClientJoinMessageID = 1,
	ClientQuitMessageID = 2,
	FreeTextMessageID = 3
};

const enum msgflgPermissions
{
	S_IRUSR = (0x00000100), //Allow the owner of the message queue to read from it.
	S_IWUSR = (0x00000080), //Allow the owner of the message queue to write to it.
	S_IRGRP = (0x00000020), //Allow the group of the message queue to read from it.
	S_IWGRP = (0x00000010), //Allow the group of the message queue to write to it.
	S_IROTH = (0x00000004), //Allow others to read from the message queue.
	S_IWOTH = (0x00000002), //Allow others to write to the message queue.
	S_ALL 	= (0x000001b6)	//Allow all
};

struct FreeTextMessage
{
	uint32_t SourceID;
	uint32_t DestinationID;
	uint32_t ReferenceID;
	uint8_t ForwardIndicator;
	char FreeText[124];
};

struct ClientJoinMessage
{
	uint32_t SourceID;
	uint32_t DestinationID;
	uint32_t PortID ;
};

struct ClientQuitMessage
{
	uint32_t SourceID;
	uint32_t DestinationID;
	uint32_t PortID ;
	uint32_t QuitType;
};

struct RKMessage
{
	int MessageID;
	char MessageStream[256];
};

void ConvertClientJoinMessageToRKMessage(const struct ClientJoinMessage* clientJoinMessage, struct RKMessage* rkMessage)
{
	rkMessage->MessageID = ClientJoinMessageID;
	memcpy(&rkMessage->MessageStream[0], &clientJoinMessage->SourceID, 4);
	memcpy(&rkMessage->MessageStream[4], &clientJoinMessage->DestinationID, 4);
	memcpy(&rkMessage->MessageStream[8], &clientJoinMessage->PortID, 4);
}

void ConvertClientQuitMessageToRKMessage(const struct ClientQuitMessage* clientQuitMessage, struct RKMessage* rkMessage)
{
	rkMessage->MessageID = ClientQuitMessageID;
	memcpy(&rkMessage->MessageStream[0], &clientQuitMessage->SourceID, 4);
	memcpy(&rkMessage->MessageStream[4], &clientQuitMessage->DestinationID, 4);
	memcpy(&rkMessage->MessageStream[8], &clientQuitMessage->PortID, 4);
	memcpy(&rkMessage->MessageStream[12], &clientQuitMessage->QuitType, 4);
}

void ConvertFreeTextMessageToRKMessage(const struct FreeTextMessage* freeTextMessage, struct RKMessage* rkMessage)
{
	rkMessage->MessageID = FreeTextMessageID;
	memcpy(&rkMessage->MessageStream[0], &freeTextMessage->SourceID, 4);
	memcpy(&rkMessage->MessageStream[4], &freeTextMessage->DestinationID, 4);
	memcpy(&rkMessage->MessageStream[8], &freeTextMessage->ReferenceID, 4);
	memcpy(&rkMessage->MessageStream[12], &freeTextMessage->ForwardIndicator, 1);
	memcpy(&rkMessage->MessageStream[13], &freeTextMessage->FreeText[0], 124);
}

void ConvertRKMessageToClientJoinMessage(const struct RKMessage* rkMessage, struct ClientJoinMessage* clientJoinMessage)
{
	memcpy(&clientJoinMessage->SourceID, &rkMessage->MessageStream[0], 4);
	memcpy(&clientJoinMessage->DestinationID, &rkMessage->MessageStream[4], 4);
	memcpy(&clientJoinMessage->PortID, &rkMessage->MessageStream[8], 4);
}

void ConvertRKMessageToClientQuitMessage(const struct RKMessage* rkMessage, struct ClientQuitMessage* clientQuitMessage)
{
	memcpy(&clientQuitMessage->SourceID, &rkMessage->MessageStream[0], 4);
	memcpy(&clientQuitMessage->DestinationID, &rkMessage->MessageStream[4], 4);
	memcpy(&clientQuitMessage->PortID, &rkMessage->MessageStream[8], 4);
	memcpy(&clientQuitMessage->QuitType, &rkMessage->MessageStream[12], 4);
}

void ConvertRKMessageToFreeTextMessage(const struct RKMessage* rkMessage, struct FreeTextMessage* freeTextMessage)
{
	memcpy(&freeTextMessage->SourceID, &rkMessage->MessageStream[0], 4);
	memcpy(&freeTextMessage->DestinationID, &rkMessage->MessageStream[4], 4);
	memcpy(&freeTextMessage->ReferenceID, &rkMessage->MessageStream[8], 4);
	memcpy(&freeTextMessage->ForwardIndicator, &rkMessage->MessageStream[12], 1);
	memcpy(&freeTextMessage->FreeText[0], &rkMessage->MessageStream[13], 124);
}

bool SendMessage(const struct RKMessage* rkMessage,const int portID)
{
	bool result = true;

    if (msgsnd(portID, rkMessage, sizeof(struct RKMessage), 0) == -1) {
        perror ("SendMessage error : ");
    	result = false;
    }

	return result;
}

bool ReadMessage(const int portID, struct RKMessage* rkMessage)
{
	bool result = true;

    if (msgrcv(portID, rkMessage, sizeof (struct RKMessage), 0, 0) == -1)
    {
        perror ("ReadMessage error: ");
        result = false;
        exit (1);
    }

	return result;
}

int CreatePort(key_t portKey)
{
	int portID = -1;

	if(portKey == 0)
	{
		portID = msgget (IPC_PRIVATE,S_ALL);
	}
	else
	{
		portID = msgget (portKey,IPC_CREAT | S_ALL);
	}


	if (portID == -1)
	{
		perror ("CreatePort error: ");
		printf("\n");
	}

	return portID;
}

int GetPortID(int portKey)
{
	int portID = -1;
	portID = msgget((key_t)portKey, 0);

	if (portID == -1)
	{
		perror ("Msgget error: ");
		printf("\n");
	}

	return portID;
}

void FreeTextMessageProcess(struct RKMessage* rkMessage)
{
	struct FreeTextMessage freeTextMessage;
	ConvertRKMessageToFreeTextMessage(rkMessage, &freeTextMessage);

	printf("FreeTextMessageProcess: Free Text is %s \n", freeTextMessage.FreeText);
}

void ProcessTermination(int Signal)
{
	if(Signal == 2)
	{
	   printf("\n ProcessTermination Function: %d \n", Signal);

	   struct RKMessage rkMessage;
	   struct ClientQuitMessage ClientQuitMessage;
	   ClientQuitMessage.SourceID = ClientSourceID;
	   ClientQuitMessage.DestinationID = ServerSourceID;
	   ClientQuitMessage.PortID = OwnOutputPortID;
	   ClientQuitMessage.QuitType = 1;

	   ConvertClientQuitMessageToRKMessage(&ClientQuitMessage, &rkMessage);

	   SendMessage(&rkMessage, OwnOutputPortID);

	   msgctl(OwnInputPortID, IPC_RMID, NULL);

	   exit(0);
	}
}


void* ListenerHandler(void* arg)
{
	struct RKMessage rkMessage;

	while(true)
	{
		if(ReadMessage(OwnInputPortID, &rkMessage))
		{
			printf("ListenerHandler: Received MessageID : %d \n", rkMessage.MessageID);

			if(rkMessage.MessageID == FreeTextMessageID)
			{
				FreeTextMessageProcess(&rkMessage);
			}
		}
		else if (GetPortID(ClientSourceID) == -1)
		{
			exit(0) ;
		}
	}

	return NULL;
}

void* ZooKeeper(void* arg)
{
	while(true)
	{
		if(GetPortID(ServerSourceID) == -1)
		{
			printf("Server kapatıldığı için Client sonlandırılıyor... \n");
			sleep(2);
			msgctl(OwnInputPortID, IPC_RMID, NULL);
			msgctl(OwnOutputPortID, IPC_RMID, NULL);
			exit(0);
		}
		sleep(10);
	}
}

bool ClientInitialize()
{
	bool result = false;

	ServerPortID = GetPortID(ServerSourceID);

	if(ServerPortID != -1)
	{
		printf("Server port is %d \n", ServerPortID);

		OwnOutputPortID = CreatePort(0);
		OwnInputPortID = CreatePort(ClientSourceID);

		if(OwnOutputPortID != -1 && OwnInputPortID != -1)
		{
			printf("Client output port is %d \n", OwnOutputPortID);
			printf("Client input port is %d \n", OwnInputPortID);
			struct ClientJoinMessage clientJoinMessage;
			clientJoinMessage.DestinationID = ServerSourceID;
			clientJoinMessage.SourceID = ClientSourceID;
			clientJoinMessage.PortID = OwnOutputPortID;

			struct RKMessage rkMessage;
			ConvertClientJoinMessageToRKMessage(&clientJoinMessage, &rkMessage);

			if(SendMessage(&rkMessage, ServerPortID) != false)
			{
				pthread_t* listenerThread = malloc(sizeof(pthread_t));
				pthread_t* zooKeeperThread = malloc(sizeof(pthread_t));

				if(listenerThread != NULL)
				{
					if((pthread_create(listenerThread, NULL, ListenerHandler, NULL) == 0) &&
							(pthread_create(zooKeeperThread, NULL, ZooKeeper, NULL) == 0)	)
					{
						result = true;
						printf("Client is initialized \n");
					}
					else
					{
						free(listenerThread);
						printf("ClientInitialize: Listener thread can't be created \n");
					}
				}
				else
				{
					printf("ClientInitialize: Listener thread can't be created because of out of memory \n");
				}
			}
			else
			{
				printf("ClientInitialize: Server couldn't be found. Maybe it is closed \n");
			}
		}
		else
		{
			printf("ClientInitialize: Client couldn't be initialized because of own port error \n");
		}
	}
	else
	{
		printf("ClientInitialize: Server couldn't be found. Maybe it is closed \n");
	}


	return result;
}

int main (int argc, char **argv)
{
	printf("Client Sa");
	ClientSourceID = (uint32_t)atoi(argv[1]);
	bool isClientInitialized = false;

	printf("Client OwnSourceID is %d \n", ClientSourceID);

	signal(SIGINT, ProcessTermination);

	if(ClientSourceID > 0 && ClientSourceID < 4000000000)
	{
		isClientInitialized = ClientInitialize();
	}

	while(isClientInitialized)
	{
		struct FreeTextMessage freeTextMessage;
		struct RKMessage rkMessage;

		freeTextMessage.SourceID = ClientSourceID;
		freeTextMessage.DestinationID = ServerSourceID;
		freeTextMessage.ForwardIndicator = 1;

		printf("Mesaj göndermek istediğiniz Client ID'sini giriniz: \n");

		char referenceID[10] = "";

		while (fgets (referenceID, 10, stdin))
		{
			freeTextMessage.ReferenceID = (uint32_t)atoi(referenceID);
			break;
		}

		printf("Gödermek istediğiniz mesajı giriniz: \n");

		while (fgets(freeTextMessage.FreeText, 124, stdin))
		{
			break;
		}

		ConvertFreeTextMessageToRKMessage(&freeTextMessage, &rkMessage);

		if(SendMessage(&rkMessage, OwnOutputPortID) == false)
		{
			if(GetPortID(ServerSourceID) == -1)
			{
				printf("Client is being terminate because of server might be closed");
			}
		}

	}

	while(true);
    exit (0);
}
