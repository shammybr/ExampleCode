// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <ThirdParty\Steamworks\Steamv147\sdk\public\steam\isteamnetworkingsockets.h>
#include <ThirdParty\Steamworks\Steamv147\sdk\public\steam\steamnetworkingtypes.h>
#include <ThirdParty\Steamworks\Steamv147\sdk\public\steam\steam_api.h>
#include "SocketManager.generated.h"




struct SMessage {
	const char* Type;
//	const void* Data;

};

struct Buffer
{
	uint8_t* data;     // pointer to buffer data
	int size;           // size of buffer data (bytes)
	int index;          // index of next byte to be read/written
	int flag;
};

UCLASS()
class BONENSOUL_API ASocketManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASocketManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	CSteamID LobbyRoom;
	CSteamID Player1;
	CSteamID OtherPlayer;
	CSteamID Player2;
	int Sock;
	uint16_t MyPCounter;
	ISteamNetworkingSockets* pInterface;
	ISteamNetworkingUtils* NetUtils;
	HSteamListenSocket ListenSocket;
	HSteamNetConnection Connection;
	TSharedPtr<class SMenuWidget> MenuWidget;
	class IOnlineSubsystem* SubSystem;
	class FOnlineSubsystemSteam* SubSystemsteam;
	int g_nVirtualPortLocal; // Used when listening, and when connecting
	int g_nVirtualPortRemote; // Only used when connecting
	bool IsLan;
	bool IsListening;
	const char* penis;
//	SteamNetworkingMessage_t* pMessages;
	//SteamNetworkingMessage_t* const* pMessages2;
	void ProcessPacket(SteamNetworkingMessage_t* _pMessage);
	STEAM_CALLBACK(ASocketManager, OnSteamNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);


	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool InitializeConection();
	bool TestInitializeConection(int baboon);
	bool InitializeSockets(bool _IsLan);
	bool SendMessage(Buffer _Buffer);
	void SetRoomPlayers(CSteamID LobbyRoom);
	void Disconnect();
	void ShutdownSockets();
	void Broadcast();

	//read and write
	bool WriteInteger(Buffer& buffer, uint32_t value);
	//void WriteShort(Buffer& buffer, uint16_t value);
	bool WriteChar(Buffer& buffer, uint8_t value);
	bool WriteFloat(Buffer& buffer, float value);
	bool WriteShort(Buffer& buffer, uint16_t value);


	uint32_t ReadInteger(Buffer& buffer);
	float ReadFloat(Buffer& buffer);
	uint16_t ReadShort(Buffer& buffer);
	uint8_t ReadByte(Buffer& buffer);

	class UBoneNSoulGameInstance* GameInstance;
	class ABoneNSoulGameSession* GameSession;
};

struct PacketStart {
	char c;

};