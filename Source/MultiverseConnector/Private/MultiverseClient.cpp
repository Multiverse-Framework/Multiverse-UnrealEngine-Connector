// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClient.h"

#include "Animation/SkeletalMeshActor.h"
#include "MultiverseClientComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiverseClient, Log, All);

// Sets default values
AMultiverseClient::AMultiverseClient()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MultiverseClientComponent = CreateDefaultSubobject<UMultiverseClientComponent>(TEXT("MultiverseClientComponent"));
}

// Called when the game starts or when spawned
void AMultiverseClient::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void AMultiverseClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MultiverseClientComponent->Deinit();

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AMultiverseClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MultiverseClientComponent->Communicate();
}

void AMultiverseClient::Init()
{
	MultiverseClientComponent->Init();
}