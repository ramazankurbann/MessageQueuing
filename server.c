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

const int ServerSourceID = 1000000;
int ServerPortID;
const int MaxClientCount = 10000;
int ClientCount = 0;

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
	char FreeText[220];
};

struct ClientJoinMessage
{
	uint32_t SourceID;
	uint32_t DestinationID;
	uint32_t PortID;
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

struct ThreadArgs
{
	struct ClientJoinMessage ClientJoinMessage;
	pthread_t* ClientThread;
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
	memcpy(&clientQuitMessage->DestinationID, &rkMessage->MessageStream[8], 4);
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
        perror ("Send message error : ");
    	result = false;
    }

	return result;
}

bool ReadMessage(const int portID, struct RKMessage* rkMessage)
{
	bool result = true;

    if (msgrcv(portID, rkMessage, sizeof (struct RKMessage), 0, 0) == -1)
    {
        perror ("Read message error: ");
        result = false;
    }

	return result;
}

int CreatePort(key_t portKey)
{
	int portID = -1;

	portID = msgget (portKey,IPC_CREAT | S_ALL);

	if (portID == -1)
	{
		perror ("Create port error: ");
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
		perror ("Get port ID error: ");
		printf("\n");
	}

	return portID;
}

void ClientQuitMessageProcess(struct RKMessage* rkMessage)
{
	struct ClientQuitMessage clientQuitMessage;
	int ownOutputPortID;

	ConvertRKMessageToClientQuitMessage(rkMessage, &clientQuitMessage);

	ownOutputPortID = clientQuitMessage.PortID;

	if(clientQuitMessage.QuitType == 1)
	{
		printf("Client(ID:%d)'ın server'dan kaydı silindi. \n", clientQuitMessage.SourceID);
		msgctl(ownOutputPortID, IPC_RMID, NULL);
	}
}

void FreeTextMessageProcess(struct RKMessage* rkMessage)
{
	struct FreeTextMessage receivedfreeTextMessage;

	ConvertRKMessageToFreeTextMessage(rkMessage, &receivedfreeTextMessage);

	if(receivedfreeTextMessage.ForwardIndicator)
	{
		struct FreeTextMessage transmitFreeTextMessage;
		struct RKMessage transmitRKMessage;

		transmitFreeTextMessage.ForwardIndicator = 0;
		transmitFreeTextMessage.SourceID = ServerSourceID;
		transmitFreeTextMessage.DestinationID = receivedfreeTextMessage.ReferenceID;
		transmitFreeTextMessage.ReferenceID = receivedfreeTextMessage.SourceID;
		memcpy(transmitFreeTextMessage.FreeText, receivedfreeTextMessage.FreeText, 124);

		ConvertFreeTextMessageToRKMessage(&transmitFreeTextMessage, &transmitRKMessage);

		uint32_t destClientID = transmitFreeTextMessage.DestinationID;

		uint32_t destClientPortID = GetPortID(destClientID);
		if(SendMessage(&transmitRKMessage, destClientPortID))
		{
			printf("Client(ID:%d), Client(ID:%d)'a mesaj gönderiyor. \n", receivedfreeTextMessage.SourceID, destClientID);
		}
	}
}

void* ClientHandler(void *threadArgs)
{
	struct ThreadArgs localThreadArgs;
	memcpy(&localThreadArgs, threadArgs, sizeof(struct ThreadArgs));

	uint32_t clientOutputportID = localThreadArgs.ClientJoinMessage.PortID;
	uint32_t clientSourceID = localThreadArgs.ClientJoinMessage.SourceID;

	while(true)
	{
		struct RKMessage rkMessage;

		if(ReadMessage(clientOutputportID, &rkMessage))
		{
			if(rkMessage.MessageID == FreeTextMessageID)
			{
				FreeTextMessageProcess(&rkMessage);
			}
			else if(rkMessage.MessageID == ClientQuitMessageID)
			{
				ClientQuitMessageProcess(&rkMessage);
				free(localThreadArgs.ClientThread);
				ClientCount--;
				pthread_exit(0) ;
			}
		}
		else if (GetPortID(clientSourceID) == -1)
		{
			ClientQuitMessageProcess(&rkMessage);
			free(localThreadArgs.ClientThread);
			ClientCount--;
			pthread_exit(0) ;
		}
	}

	return NULL;
}


void ClientJoinMessageProcess(struct RKMessage* rkMessage)
{
	pthread_t* clientThread = malloc(sizeof(pthread_t));
	struct ThreadArgs threadArgs;
	struct ClientJoinMessage clientJoinMessage;

	if(clientThread != NULL)
	{
		ConvertRKMessageToClientJoinMessage(rkMessage, &clientJoinMessage);

		threadArgs.ClientThread = clientThread;
		memcpy(&threadArgs.ClientJoinMessage, &clientJoinMessage, sizeof(struct ClientJoinMessage));

		if(pthread_create(clientThread, NULL, ClientHandler, &threadArgs) == 0)
		{
			printf("Client(ID:%d) kayıt edildi. \n", clientJoinMessage.SourceID);
		}
		else
		{
			free(clientThread);
			printf("Client oluşturulamadı. \n");
			ClientCount--;
		}
	}
	else
	{
		printf("Client oluşturulamadı. Hafıza eksik olabilir. \n");
		ClientCount--;
	}
}

void ProcessTermination(int Signal)
{
	if(Signal == 2)
	{
	   printf("Server yok ediliyor...\n");
	   msgctl(ServerPortID, IPC_RMID, NULL);
	   exit(0);
	}
}

int main (int argc, char **argv)
{
	ServerPortID = CreatePort(ServerSourceID);
	signal(SIGINT, ProcessTermination);

	struct RKMessage rkMessage;

	if(ServerPortID != -1)
	{
		printf("Server başlatıldı. \n");

		while (1)
		{
			if(ReadMessage(ServerPortID, &rkMessage))
			{
				if(rkMessage.MessageID == ClientJoinMessageID && ClientCount < MaxClientCount)
				{
					ClientJoinMessageProcess(&rkMessage);
					ClientCount++;
				}
			}
			else if(GetPortID(ServerSourceID) == -1)
			{
				printf("Maksimum Client sayısına ulaşıldı. Yeni Client kayıt edilemez. \n");
				break;
			}
		}
	}

	printf("Server portu kapandığı için yeni kayıt alınamıyor. Sadece client portları işlem görüyor. \n");

	while(ClientCount > 1);

	printf("Server işlevsiz kaldığı için kapanıyor... \n");

	return 0;
}
