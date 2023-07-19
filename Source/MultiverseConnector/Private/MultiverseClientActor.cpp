// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientActor.h"

#include "Animation/SkeletalMeshActor.h"
#include "MultiverseClientComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiverseClient, Log, All);

// Sets default values
AMultiverseClientActor::AMultiverseClientActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MultiverseClientComponent = CreateDefaultSubobject<UMultiverseClientComponent>(TEXT("MultiverseClientComponent"));
}

// Called when the game starts or when spawned
void AMultiverseClientActor::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void AMultiverseClientActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MultiverseClientComponent->Deinit();

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AMultiverseClientActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MultiverseClientComponent->Tick();
}

void AMultiverseClientActor::Init()
{
	MultiverseClientComponent->Init();
}