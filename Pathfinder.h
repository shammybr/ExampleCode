// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <vector>
#include <unordered_map>
#include "Pathfinder.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BONENSOUL_API UPathfinder : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPathfinder();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void UpdatePlatformMap(TArray<class APlatform*> _RoomPlatforms);

	TArray<int> FindPath(int From, int To);
		

//	TArray<class APlatform*> RoomPlatforms;
	std::unordered_map<int, class APlatform*> RoomPlatforms;
	//Distance from platforms
	std::vector<std::vector<float>> PlatformDistanceToPlatform;
};
