// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinder.h"
#include "APlatform.h"
#include <queue>
#include <unordered_map>

// Sets default values for this component's properties
UPathfinder::UPathfinder()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPathfinder::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UPathfinder::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UPathfinder::UpdatePlatformMap(TArray<APlatform*> _RoomPlatforms)
{
	
//	RoomPlatforms = _RoomPlatforms;
	PlatformDistanceToPlatform.clear();
	RoomPlatforms.clear();

//	UE_LOG(LogTemp, Error, TEXT("RoomPlatforms %s"), *GetOwner()->GetName());
//  UE_LOG(LogTemp, Error, TEXT("RoomPlatforms %d"), _RoomPlatforms.Num());

	//for (int i = 0; i < RoomPlatforms.Num(); i++) {
	//	PlatformDistanceToPlatform.AddZeroed();
	//	

	//}
	if (_RoomPlatforms.Num() > 1) {
		PlatformDistanceToPlatform.resize(_RoomPlatforms.Num());
		for (int i = 0; i < _RoomPlatforms.Num(); i++) {
			RoomPlatforms[_RoomPlatforms[i]->PlatformID] = _RoomPlatforms[i];
			PlatformDistanceToPlatform[i].resize(_RoomPlatforms.Num());

		}

		for (std::pair<int, APlatform*> Platform : RoomPlatforms) {



			for (FNeighbor Neighbor : Platform.second->Neighbors) {

				//Distance from platforms
				PlatformDistanceToPlatform[Platform.second->PlatformID][Neighbor.PlatformID] = FVector::Distance(RoomPlatforms[Neighbor.PlatformID]->GetActorLocation(), Platform.second->GetActorLocation());

			}
		}
	}

}

TArray<int> UPathfinder::FindPath(int From, int To) {

	TArray<int> Path;
	if (From == To) {
		return Path;
	}

	//list to search
	TArray<APlatform*> OpenList;
	
	

	//from platform to platform
	std::unordered_map<int, int> CameFrom;


	//platform, distance to goal
	TArray<float> PlatformTotalDistance;

	//platform distance to goal
	TArray<float> PlatformDistanceToGoal;



	for (int i = 0; i < RoomPlatforms.size(); i++) {
		PlatformTotalDistance.Add(-1.0f);
		PlatformDistanceToGoal.Add(-1.0f);
		OpenList.Add(NULL);
		
	}

	for (std::pair<int, APlatform*> Platform : RoomPlatforms) {
		OpenList[Platform.second->PlatformID] = Platform.second;


		PlatformDistanceToGoal[Platform.second->PlatformID] = FVector::Distance(RoomPlatforms[Platform.second->PlatformID]->GetActorLocation(), RoomPlatforms[To]->GetActorLocation());
				
		

	}


	//searched list
	std::vector<bool> ClosedList;
	ClosedList.resize(RoomPlatforms.size(), false);


	//the smaller distance travelled
	float SmallestDistanceTraveled = 999999.0f;

	int CurrentPlatform = From;

	if (RoomPlatforms.size() > 0) {
		PlatformTotalDistance[From] = PlatformDistanceToGoal[From];


		//							Distance - item
		std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> PathQueue;

		PathQueue.push(std::make_pair(PlatformDistanceToGoal[From], From));



		while (!PathQueue.empty()) {

			CurrentPlatform = PathQueue.top().second;
			UE_LOG(LogTemp, Error, TEXT("RoomPlatforms %d"), PathQueue.top().second);
			PathQueue.pop();

			ClosedList[CurrentPlatform] = true;

			if (CurrentPlatform == To) {
				break;
			}



			for (FNeighbor Neighbor : RoomPlatforms[CurrentPlatform]->Neighbors) {

				float NewDistance = PlatformTotalDistance[CurrentPlatform] + PlatformDistanceToPlatform[CurrentPlatform][Neighbor.PlatformID] + PlatformDistanceToGoal[Neighbor.PlatformID];

				if (NewDistance < PlatformTotalDistance[Neighbor.PlatformID] || PlatformTotalDistance[Neighbor.PlatformID] == -1.0f) {
					CameFrom[Neighbor.PlatformID] = CurrentPlatform;
					PlatformTotalDistance[Neighbor.PlatformID] = NewDistance;
				}
				if (!ClosedList[Neighbor.PlatformID]) {
					PathQueue.push(std::make_pair(PlatformTotalDistance[Neighbor.PlatformID], Neighbor.PlatformID));
				}
			}

		}

		int iterator = 1;
		Path.Add(To);
		int i = CameFrom[To];
		Path.Insert(i, 0);
		while (i != From) {
			i = CameFrom[i];
			Path.Insert(i, 0);

		}
	}
	return Path;

}
