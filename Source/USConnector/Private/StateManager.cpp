// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "StateManager.h"

#include "Animation/SkeletalMeshActor.h"
#include "StateController.h"

DEFINE_LOG_CATEGORY_STATIC(LogStateManager, Log, All);

// Sets default values
AStateManager::AStateManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StateController = CreateDefaultSubobject<UStateController>(TEXT("StateController"));
}

// Called when the game starts or when spawned
void AStateManager::BeginPlay()
{
	Super::BeginPlay();

	Init();
}

void AStateManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StateController->Deinit();

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AStateManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	StateController->Tick();
}

void AStateManager::Init()
{
	StateController->Init();
}