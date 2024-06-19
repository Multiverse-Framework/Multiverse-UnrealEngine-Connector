// Copyright (c) 2023, Giang Hoang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientActor.h"

#include "MultiverseClientComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiverseClientActor, Log, All);

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

	MultiverseClientComponent->Tick(DeltaTime);
}

void AMultiverseClientActor::Init() const
{
	UWorld *World = GetWorld();
	if (!World)
	{
		UE_LOG(LogMultiverseClientActor, Error, TEXT("World not found"));
		return;
	}

#if WITH_EDITOR
	for (AActor *Actor : World->GetCurrentLevel()->Actors)
	{
		if (Actor && Actor->Tags.Contains((FName("receive_position"))))
		{
			FAttributeContainer AttributeContainer;
			AttributeContainer.ObjectName = Actor->GetActorLabel();
			AttributeContainer.Attributes.Add(EAttribute::Position);
			MultiverseClientComponent->ReceiveObjects.Add(Actor, AttributeContainer);
		}
		if (Actor && Actor->Tags.Contains((FName("receive_quaternion"))))
		{
			FAttributeContainer AttributeContainer;
			AttributeContainer.ObjectName = Actor->GetActorLabel();
			AttributeContainer.Attributes.Add(EAttribute::Quaternion);
			MultiverseClientComponent->ReceiveObjects.Add(Actor, AttributeContainer);
		}
	}
#endif

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	FAttributeContainer AttributeContainer;
	AttributeContainer.ObjectName = TEXT("PlayerPawn");
	AttributeContainer.Attributes.Add(EAttribute::Position);
	AttributeContainer.Attributes.Add(EAttribute::Quaternion);
	MultiverseClientComponent->SendObjects.Add(PlayerPawn, AttributeContainer);

	MultiverseClientComponent->Init();
}
