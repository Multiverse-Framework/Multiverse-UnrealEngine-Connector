// Copyright (c) 2022, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "GameFramework/Actor.h"
// clang-format off
#include "MultiverseClientActor.generated.h"
// clang-format on

class UMultiverseClientComponent;

UCLASS()
class MULTIVERSECONNECTOR_API AMultiverseClientActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMultiverseClientActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Overridable function called whenever this actor is being removed from a level */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void Init();

private:
	UPROPERTY(VisibleAnywhere, Category = "Multiverse Client")
	UMultiverseClientComponent *MultiverseClientComponent;
};

