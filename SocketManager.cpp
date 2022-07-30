// Fill out your copyright notice in the Description page of Project Settings.

// UE4: allow Windows platform types to avoid naming collisions
//  this must be undone at the bottom of this file




#include "SocketManager.h"
#ifdef _MSC_VER
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include <BoneNSoul/BoneNSoulGameInstance.h>
#include <BoneNSoul/BoneNSoulGameSession.h>
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include <ExamplePlugin.h>
#include <winsock2.h>
#include <BoneNSoulHUD.h>
#include <Source/Public/OnlineSubsystemSteam.h>
#include <iostream>
#include <fstream>
#include <ShlObj_core.h>
#include <pathcch.h>
#include <sstream>


#undef SendMessage

//namespace packetHeader {
//	const char STARTGAME = 's';
//	const char CHANGECHARACTER = 'c';
//	const char SENDPOSITION = 'p';
//	const char SENDPOSITIONATTACKING = 'a';
//	const char REQUESTROOMDATA = 'r';
//	const char SENDROOMDATA = 'm';
//	const char SENDPLAYERROOM = 'q';
//
//}

// Sets default values
ASocketManager::ASocketManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASocketManager::BeginPlay()
{
	Super::BeginPlay();

	GameInstance = Cast<UBoneNSoulGameInstance>(GetGameInstance());
	GameSession = Cast<ABoneNSoulGameSession>(GameInstance->GetGameSession());
	MyPCounter = 0;
	GameSession->SetSocketManager(this);

	pInterface = SteamNetworkingSockets();
	NetUtils = SteamNetworkingUtils();
	ListenSocket = k_HSteamListenSocket_Invalid;
	Connection = k_HSteamNetConnection_Invalid;
	SubSystem = IOnlineSubsystem::Get();


	//steam
	if (SubSystem->GetSubsystemName() == TEXT("Steam")) {
		SteamNetworkingUtils()->InitRelayNetworkAccess();

	}



	g_nVirtualPortLocal = 0; // Used when listening, and when connecting
	g_nVirtualPortRemote = 0; // Only used when connecting
}

// Called every frame
void ASocketManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (Connection != k_HSteamNetConnection_Invalid) {
		SteamNetworkingMessage_t* pMessage;
		int r = pInterface->ReceiveMessagesOnConnection(Connection, &pMessage, 1);
		if (r < 0) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Connection error")));

		}
		else if (r >= 1) {
		//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Message received")));
	//		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("SIZE %i"), pMessage->m_cbSize));
				ProcessPacket(pMessage);
			


		}
	}

	else if (IsLan && IsListening) {
		struct sockaddr_in Sender_addr;

		int len = sizeof(struct sockaddr_in);

		char recvbuff[1];
		int recvbufflen = 1;

		int BytesReceived = recvfrom(Sock, recvbuff, recvbufflen, 0, (sockaddr*)&Sender_addr, &len);

		if (BytesReceived == SOCKET_ERROR)
		{


		}

		else {
			UE_LOG(LogOnlineGame, Warning, TEXT("Received didn't fail: %d"), WSAGetLastError);
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Not Fail")));
			/*if (sendto(Sock, sendMSG, strlen(sendMSG) + 1, 0, (sockaddr*)&Sender_addr, sizeof(Sender_addr)) < 0)
			{
				closesocket(Sock);
			}*/

		}
	}
	
}

void ASocketManager::ProcessPacket(SteamNetworkingMessage_t* _pMessage) {


	Buffer MBuffer = {
	MBuffer.data = (uint8_t*)_pMessage->m_pData,
	MBuffer.size = _pMessage->m_cbSize,
	MBuffer.index = 0};
//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Message received %i"), _pMessage->m_cbSize));
	char CByte = ReadByte(MBuffer);

	switch (CByte) {
		//start game
		case (packetHeader::STARTGAME): {




		ABoneNSoulHUD* HUD = Cast<ABoneNSoulHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
		if (HUD->GetMenuWidget()->Status == SMenuWidget::MenuStatus::JoinedMenu) {
			
			char CByte2 = ReadByte(MBuffer);

			if (CByte2 == 'g') {

				HUD->GetMenuWidget()->ForceChangeCharacter(EPlayerCharacter::Ghost);

			}
			else if (CByte2 == 's') {

				HUD->GetMenuWidget()->ForceChangeCharacter(EPlayerCharacter::Skeleton);

			}
			
			FLatentActionInfo info;
			GameInstance->ChangeGameMode(EGameMode::Multiplayer);
			UGameplayStatics::LoadStreamLevel(GetWorld(), "FirstLevel", true, true, info);

		}


		break;




	}
		//change lobby character
		case (packetHeader::CHANGECHARACTER): {
		ABoneNSoulHUD* HUD = Cast<ABoneNSoulHUD>(GetWorld()->GetFirstPlayerController()->GetHUD());
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("c")));
		if (HUD->GetMenuWidget()->Status == SMenuWidget::MenuStatus::JoinedMenu) {

			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("joined")));
			
			char CByte2 = ReadByte(MBuffer);

			if (CByte2 == 'g') {

				HUD->GetMenuWidget()->ForceChangeCharacter(EPlayerCharacter::Ghost);


			}
			else if (CByte2 == 's') {

				HUD->GetMenuWidget()->ForceChangeCharacter(EPlayerCharacter::Skeleton);


			}
		}

		break;
	}
		//position other player
		case(packetHeader::POSITION): {

			uint16_t PCounter = ReadShort(MBuffer);

			if (PCounter > MyPCounter || ((PCounter < 20000) && (MyPCounter > 50000))) {

				MyPCounter = PCounter;

				float x = ReadFloat(MBuffer);
				float z = ReadFloat(MBuffer);
				unsigned char Animation = ReadByte(MBuffer);
				uint8_t RightAndHurting = ReadByte(MBuffer);
				
				GameInstance->SetPawnPosition(x, z, Animation, PCounter, false, RightAndHurting);

			}
			break;
		}

		//attack
		case (packetHeader::POSITIONATTACKING): {

			uint16_t PCounter = ReadShort(MBuffer);
			if (PCounter > MyPCounter || ((PCounter < 20000) && (MyPCounter > 50000))) {
				MyPCounter = PCounter;

				float x = ReadFloat(MBuffer);
				float z = ReadFloat(MBuffer);
				unsigned char Animation = ReadByte(MBuffer);
				uint8_t RightAndHurting = ReadByte(MBuffer);
				
				GameInstance->SetPawnPosition(x, z, Animation, PCounter, true, RightAndHurting);

			}
			else {
				float x = ReadFloat(MBuffer);
				float z = ReadFloat(MBuffer);
				unsigned char Animation = ReadByte(MBuffer);
				uint8_t RightAndHurting = ReadByte(MBuffer);
				
				GameInstance->PastAttack(x, z, Animation, RightAndHurting);

			}

			break;
		}

		case(packetHeader::ANSWERREQUEST): {

			
			if (GameInstance) {
			

					unsigned char AnswerID = ReadByte(MBuffer);

					switch (AnswerID) {
						//Can spawn room
						case 0:

							GameInstance->SpawnRoomEnemies();
						break;

				}
			}

		  break;
	    }
	
		case(packetHeader::ROOMDATA): {
			
			if (GameInstance) {
				if (GameSession) {	            
					unsigned char PawnMapSize = ReadByte(MBuffer);
					for(int i = 0; i < PawnMapSize; i++){
					
						unsigned char ClassID = ReadByte(MBuffer);
						uint8_t NetworkID = ReadByte(MBuffer);
						uint32_t Health = ReadInteger(MBuffer);
						float x = ReadFloat(MBuffer);
						float z = ReadFloat(MBuffer);
						uint16_t Rotation = ReadShort(MBuffer);

						GameSession->SpawnNetworkPawn(ClassID, NetworkID, x, z, Health, Rotation);
					}
					
					for (int i = MBuffer.index; i + 14 <= MBuffer.size; ) {

						uint8_t PickableID = ReadByte(MBuffer);
						uint8_t ItemID = ReadByte(MBuffer);
						uint8_t NetworkID = ReadByte(MBuffer);
						float x = ReadFloat(MBuffer);
						float z = ReadFloat(MBuffer);

						GameSession->SpawnNetworkPickable(PickableID, NetworkID, ItemID, x, z);
						i = MBuffer.index;
					}

					GameInstance->SpawnPermanents();
					GameSession->IsRequestingRoomData = false;
				}
			}

			


			break;
		}

		case(packetHeader::PLAYERROOM): {

			uint32_t RoomNumber = ReadInteger(MBuffer);
			unsigned char WannaSpawnChar = ReadByte(MBuffer);
			

			if (GameInstance) {
				
				
				bool WannaSpawn;
				if (WannaSpawnChar == 't') {

					WannaSpawn = true;

				}
				else {
					WannaSpawn = false;
				}
				GameInstance->SetPlayerRoom(GameInstance->GetOtherPawn(), RoomNumber, WannaSpawn);
				

			}


			break;
		}

		case(packetHeader::MONSTERPOSITION): {

			uint8_t NetworkID = ReadByte(MBuffer);
			uint8_t Animation = ReadByte(MBuffer);
			uint8_t RightAndHurting = ReadByte(MBuffer);
			float x = ReadFloat(MBuffer);
			float z = ReadFloat(MBuffer);
			uint16_t PCounter = ReadShort(MBuffer);
			

			GameInstance->SetMonsterPosition(x, z, Animation, NetworkID, PCounter, RightAndHurting, false);

			break;
		}

		case(packetHeader::MONSTERPOSITIONATTACKING): {

			uint32_t NetworkID = ReadByte(MBuffer);
			uint8_t Animation = ReadByte(MBuffer);
			uint8_t RightAndHurting = ReadByte(MBuffer);
			float x = ReadFloat(MBuffer);
			float z = ReadFloat(MBuffer);
			uint16_t PCounter = ReadShort(MBuffer);

			
			GameInstance->SetMonsterPosition(x, z, Animation, NetworkID, PCounter, RightAndHurting, true);

			break;
		}

		case (packetHeader::MONSTEREFFECT): {

			uint32_t NetworkID = ReadByte(MBuffer);
			float x = ReadFloat(MBuffer);
			float z = ReadFloat(MBuffer);
			uint8_t Effect = ReadByte(MBuffer);

			
			GameInstance->ApplyMonsterEffect(FVector(x, 0, z), NetworkID, Effect);




		}
		case (packetHeader::MONSTERDAMAGETAKEN): {

			uint8_t NetworkID = ReadByte(MBuffer);
			float Damage = ReadFloat(MBuffer);


			
			GameInstance->DamageNetworkMonster(NetworkID, Damage);
			break;
		}
		case (packetHeader::DAMAGETAKEN): {

			float Damage = ReadFloat(MBuffer);


			
			GameInstance->DamageNetworkPawn(Damage);
			break;
		}
		case (packetHeader::SPELLEFFECT): {
			

			uint32_t NetworkID = ReadByte(MBuffer);
			uint32_t SpellEffectInt = ReadInteger(MBuffer);
			uint32_t SpellEffectMod = ReadInteger(MBuffer);
			ESpellEffect SpellEffect = static_cast<ESpellEffect>(SpellEffectInt);
			uint8_t ClassID; 
			UE_LOG(LogOnlineGame, Warning, TEXT("SpellEffect %d"), SpellEffectInt);
			switch (SpellEffect) {
				case NETWORKPOSSESS:
					
				   ClassID = ReadByte(MBuffer);
					if (GameInstance) {
						GameInstance->NetworkPossess(NetworkID, ClassID);
					}
				
				

				break;

				case NETWORKUNPOSSESS:
					if (GameInstance) {
						GameInstance->NetworkUnpossess();
					}

				break;

				case NETWORKHEAL:
					ClassID = ReadByte(MBuffer);
					if (GameInstance) {
						GameInstance->NetworkHeal(NetworkID, ClassID, SpellEffectMod);
					}

				break;
				case NETWORKDEBUFF:
					ClassID = ReadByte(MBuffer);
					if (GameInstance) {
						GameInstance->ApplyNetworkDebuff(NetworkID, ClassID, SpellEffectMod);
					}

				break;
			}
			
		//	GameInstance->DamageNetworkPawn(Damage);
			break;
		}

		case (packetHeader::SPECIALEFFECT): {
			

				float x = ReadFloat(MBuffer);
				float z = ReadFloat(MBuffer);
				uint8_t Effect = ReadByte(MBuffer);
				uint8_t Animation = ReadByte(MBuffer);
				uint8_t LookingRight = ReadByte(MBuffer);
				float EffectMod = ReadFloat(MBuffer);

				if (GameInstance) {
					GameInstance->SpecialEffect(x, z, Animation, LookingRight, Effect, EffectMod);
				}

			break;
		}

		case (packetHeader::CASTINGPOSITION): {

			uint16_t PCounter = ReadShort(MBuffer);
			if (PCounter > MyPCounter || ((PCounter < 20000) && (MyPCounter > 50000))) {
				MyPCounter = PCounter;

				float x = ReadFloat(MBuffer);
				float z = ReadFloat(MBuffer);
				uint16_t SpellID = ReadShort(MBuffer);
				unsigned char Animation = ReadByte(MBuffer);
				uint8_t RightAndHurting = ReadByte(MBuffer);
				
				
				GameInstance->SetPawnCastingPosition(x, z, Animation, SpellID, RightAndHurting);

			}



			break;
		}

		case (packetHeader::LOOTMANAGEMENT): {

		//	
			
			

			if (MBuffer.size == 3) {

				uint8_t IsPermanent = ReadByte(MBuffer);
				uint8_t PickUPMapID = ReadByte(MBuffer);

				if (IsPermanent == 0) {
					GameInstance->UnregisterPermanentPowerUp(PickUPMapID);
				}
				else {
					GameInstance->UnregisterItem(PickUPMapID);
				}


			}
			else {

				uint8_t PickUPMapID = ReadByte(MBuffer);
				uint8_t PickableID = ReadByte(MBuffer);
				uint8_t ItemID = ReadByte(MBuffer);
				float x = ReadFloat(MBuffer);
				float z = ReadFloat(MBuffer);
				GameInstance->SpawnNetworkedItem(PickableID, PickUPMapID, ItemID, x, z);
			}


			break;

		}
		case (packetHeader::LEARNSPELL): {
			uint8_t ExtraCharges = ReadByte(MBuffer);
			uint8_t ExtraSlots = ReadByte(MBuffer);
			float SpellID = ReadFloat(MBuffer);

			
			GameInstance->LearnNetworkSpell(SpellID, ExtraCharges, ExtraSlots);

			





			break;
		
		}
		case (packetHeader::PERMANENTCHANGE): {
			uint8_t PermanentType = ReadByte(MBuffer);
			uint8_t PermanentID = ReadByte(MBuffer);
			uint8_t Change = ReadByte(MBuffer);
			if (GameInstance) {
				if (Change == 0) {
					switch (PermanentType) {
						case 0:
							GameInstance->UnregisterBarrier(PermanentID);

						break;

						case 1:
							GameInstance->UnregisterUniqueEnemy(PermanentID);

						break;
					}
				
				}
				else {


				}
			}

			break;
		}

		case (packetHeader::PING): {
			uint8_t PingType = ReadByte(MBuffer);
			uint8_t PingID = ReadByte(MBuffer);

			if (PingType == 0) {
				GameSession->Pong(PingID);
	//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Pong Sent %i"), PingID));
			}
			else {
				GameInstance->ProcessPong(PingID);
	//			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Process pong %i"), PingID));
			}

			break;
		}

		case (packetHeader::CHANGEROOM): {

			MyPCounter = ReadShort(MBuffer);
			float x = ReadFloat(MBuffer);
			float z = ReadFloat(MBuffer);
			uint8_t Animation = ReadByte(MBuffer);
			uint8_t LookingRight = ReadByte(MBuffer);
			uint16_t RoomSwitcher = ReadShort(MBuffer);

			GameInstance->ApplyRoomChange(x, z, Animation, LookingRight, RoomSwitcher);


			break;
		}


	}

}

bool ASocketManager::InitializeConection() {




	
	if (SteamMatchmaking()->GetNumLobbyMembers(LobbyRoom) == 2) {
		if (SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyMemberByIndex(LobbyRoom, 0)) {

			OtherPlayer = SteamMatchmaking()->GetLobbyMemberByIndex(LobbyRoom, 1);

		}
		else {

			OtherPlayer = SteamMatchmaking()->GetLobbyMemberByIndex(LobbyRoom, 0);

		}

		SteamNetworkingIdentity identityLocal; identityLocal.Clear();
		SteamNetworkingIdentity identityRemote; identityRemote.Clear();

		g_nVirtualPortRemote = 0;

		identityRemote.SetSteamID(OtherPlayer);

		Connection = pInterface->ConnectP2P(identityRemote, g_nVirtualPortRemote, 0, nullptr);
		
	}

	else {

		SteamNetworkingConfigValue_t opt;
		opt.SetInt32(k_ESteamNetworkingConfig_SymmetricConnect, 1);
		ListenSocket = pInterface->CreateListenSocketP2P(g_nVirtualPortLocal, 1, &opt);



	}

	return true;

}

bool ASocketManager::TestInitializeConection(int baboon) {

	if (baboon) {
		SteamNetworkingConfigValue_t opt;
		SteamNetworkingIPAddr IPAddr;
		IPAddr.Clear();
		IPAddr.m_port = 8081;

		
		opt.SetInt32(k_ESteamNetworkingConfig_SymmetricConnect, 1);
		ListenSocket = SteamNetworkingSockets()->CreateListenSocketIP(IPAddr, 1, &opt);


		
	}
	else {

		SteamNetworkingConfigValue_t opt;
		SteamNetworkingIPAddr IPAddr;
		IPAddr.SetIPv4(127001, 8081);

		opt.SetInt32(k_ESteamNetworkingConfig_SymmetricConnect, 1);
		ListenSocket = pInterface->ConnectByIPAddress(IPAddr, 1, &opt);
		//SteamNetworkingIPAddr IPAddr;
	
		//

		//if (IPAddr.IsLocalHost()) {
		//	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("IPV4: %i"), IPAddr.GetIPv4()));
		//}
		
	}

	return true;
}

void FreeMessage(SteamNetworkingMessage_t *pMsg) {

//	AsyncTask(ENamedThreads::GameThread, [=]()
	//	{

			free(pMsg->m_pData);
			
	//	});

	
}

bool ASocketManager::SendMessage(Buffer _Buffer) {

	if (Connection != k_HSteamNetConnection_Invalid) {
	//EResult r  = pInterface->SendMessageToConnection(Connection, Message, sizeof(Message), k_nSteamNetworkingSend_Reliable, nullptr);
	SteamNetworkingMessage_t* pMessages = NetUtils->AllocateMessage(0);
	pMessages->m_pData = _Buffer.data;
	pMessages->m_cbSize = _Buffer.size;
	pMessages->m_pfnFreeData = FreeMessage;
	pMessages->m_conn = Connection;
	pMessages->m_nFlags = _Buffer.flag;
	SteamNetworkingMessage_t* const* pMessages2 = &pMessages;
	int64* pOutMessageNumberOrResult = new int64;

	
		pInterface->SendMessages(1, pMessages2, pOutMessageNumberOrResult);
		if (pOutMessageNumberOrResult >= 0) {
			return true;
			
		}
		else {
			return false;
			
		}
	}
	return false;
}



void ASocketManager::SetRoomPlayers(CSteamID _LobbyRoom) {
	LobbyRoom = _LobbyRoom;
	Player1 = SteamMatchmaking()->GetLobbyMemberByIndex(_LobbyRoom, 0);
	Player2 = SteamMatchmaking()->GetLobbyMemberByIndex(_LobbyRoom, 1);

}


void ASocketManager::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{

	UE_LOG(LogOnlineGame, Warning, TEXT("Changed: %d"), pInfo->m_info.m_eState);
	switch (pInfo->m_info.m_eState) {
		
		case k_ESteamNetworkingConnectionState_ClosedByPeer:
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Closed By Peer")));
			UE_LOG(LogOnlineGame, Warning, TEXT("TEST1"));
			// Close our end
			if (pInfo->m_hConn != k_HSteamNetConnection_Invalid) {
		//		pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);



				SteamNetworkingSockets()->CloseConnection(pInfo->m_hConn, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);

					
				
				Connection = k_HSteamNetConnection_Invalid;
				UE_LOG(LogOnlineGame, Warning, TEXT("TEST2"));
			}
			UE_LOG(LogOnlineGame, Warning, TEXT("TEST3"));
				
			break;
		case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Problem Detected Locally")));

			if (pInfo->m_hConn != k_HSteamNetConnection_Invalid) {
				// Close our end

				SteamNetworkingSockets()->CloseConnection(pInfo->m_hConn, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);
				Connection = k_HSteamNetConnection_Invalid;
			

					
					
				
			}
			
			
			break;

		case k_ESteamNetworkingConnectionState_Connecting:
			Connection = pInfo->m_hConn;
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Accepting connection1")));
			if (ListenSocket != k_HSteamListenSocket_Invalid && ListenSocket == pInfo->m_info.m_hListenSocket) {
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Accepting connection2")));
				Connection = pInfo->m_hConn;
				SteamNetworkingSockets()->AcceptConnection(pInfo->m_hConn);


			}
			break;

		case k_ESteamNetworkingConnectionState_Connected:
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Connected")));
			break;

		default:
			UE_LOG(LogOnlineGame, Warning, TEXT("TEST4"));
			break;

	}
	




}


void ASocketManager::ShutdownSockets() {


}

void ASocketManager::Broadcast() {


}


void ASocketManager::Disconnect(){
	if (Connection != k_HSteamNetConnection_Invalid) {
		// Close our end
		
				pInterface->CloseConnection(Connection, 0, nullptr, false);
				Connection = k_HSteamNetConnection_Invalid;
		
	}



}

bool ASocketManager::WriteInteger(Buffer& buffer, uint32_t value)
{
	if (buffer.index + 4 <= buffer.size) {

		*((uint32_t*)(buffer.data + buffer.index)) = htonl(value);

		buffer.index += 4;

		return true;
	}
	else {
		return false;

	}
}

bool ASocketManager::WriteShort(Buffer& buffer, uint16_t value) {

	if (buffer.index + 2 <= buffer.size) {
		*((uint16_t*)(buffer.data + buffer.index)) = value;

		buffer.index += 2;
		return true;

	}

	else {
		return false;
	}


}

uint32_t ASocketManager::ReadInteger(Buffer& buffer)
{
	if (buffer.index + 4 <= buffer.size) {
		uint32_t value;
		value = ntohl(*((uint32_t*)(buffer.data + buffer.index)));
		buffer.index += 4;
		return value;
	}

	else {

		return 0;
	}



}


bool ASocketManager::WriteChar(Buffer& buffer, uint8_t value) {
	if (buffer.index + 1 <= buffer.size) {

		*((uint8_t*)(buffer.data + buffer.index)) = value;
		buffer.index += 1;
		return true;
	}

	else {
		return false;
	}
	
}

uint8_t ASocketManager::ReadByte(Buffer& buffer) {

	if (buffer.index + 1 <= buffer.size) {
//		uint8_t value;
		uint8_t valuec;
		valuec = (*((uint8_t*)(buffer.data + buffer.index)));
		buffer.index += 1;
		return valuec;
	}

	else {
		return 0;
	}
	
}

bool ASocketManager::WriteFloat(Buffer& buffer, float value) {
	if (buffer.index + 4 <= buffer.size) {
		union FloatInt
		{
			float float_value;
			uint32_t int_value;
		};

		FloatInt tmp;
		tmp.float_value = value;
		*((uint32_t*)(buffer.data + buffer.index)) = tmp.int_value;

		buffer.index += 4;

		return true;
		

		
		return true;
	}
	return false;
}

float ASocketManager::ReadFloat(Buffer& buffer) {

	if (buffer.index + 4 <= buffer.size) {
		float value;
		value = (*((float*)(buffer.data + buffer.index)));
		buffer.index += 4;
		return value;
	}

	else {

		return 0;
	}

}


uint16_t ASocketManager::ReadShort(Buffer& buffer) {

	if (buffer.index + 2 <= buffer.size) {
		uint16_t value;
		value = (*((uint16_t*)(buffer.data + buffer.index)));
		buffer.index += 2;
		return value;
	}

	else {

		return 0;
	}






}


bool ASocketManager::InitializeSockets(bool _IsLan) {



	WORD winsock_version = 0x202;
	WSADATA winsock_data;
	if (WSAStartup(winsock_version, &winsock_data) != 0)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("WSAStartup failed: %d"), WSAGetLastError);
		
		return false;
	}
	
	UE_LOG(LogOnlineGame, Warning, TEXT("WSAStartup"), WSAGetLastError);
	int address_family = AF_INET;
	int type = SOCK_DGRAM;
	int protocol = IPPROTO_UDP;
	//create
	Sock = socket(address_family, type, protocol);

	if (Sock == INVALID_SOCKET)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("Socket failed: %d"), WSAGetLastError);
		return false;
	}

	UE_LOG(LogOnlineGame, Warning, TEXT("SOCK"), WSAGetLastError);
	char broadcast = '1';
	if (setsockopt(Sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
	{
		closesocket(Sock);
		return false;
	}

	DWORD nonBlocking = 1;
	if (ioctlsocket(Sock,FIONBIO,&nonBlocking) != 0)
	{
		return false;
	}
	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("SOCKOPT")));
	UE_LOG(LogOnlineGame, Warning, TEXT("SOCKOPT"), WSAGetLastError);
	//bind
	SOCKADDR_IN local_address;
	local_address.sin_family = AF_INET;
	local_address.sin_port = htons(0);
	local_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(Sock, (SOCKADDR*)&local_address, sizeof(local_address)) == SOCKET_ERROR)
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("Bind failed: %d"), WSAGetLastError);
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Bind failed")));
		return false;
	}

	IsLan = _IsLan;
	IsListening = true;
	

	return true;
}


// UE4: disallow windows platform types
//  this was enabled at the top of the file
#ifdef _MSC_VER
#include "Windows/HideWindowsPlatformTypes.h"
#endif