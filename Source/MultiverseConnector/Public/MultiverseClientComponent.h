// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "CoreMinimal.h"

// clang-format off
#include "MultiverseClientComponent.generated.h"
// clang-format on

struct FAttributeContainer;
class FMultiverseClient;

UCLASS(Blueprintable, DefaultToInstanced, collapsecategories, hidecategories = Object, editinlinenew)
class MULTIVERSECONNECTOR_API UMultiverseClientComponent final : public UObject
{
	GENERATED_BODY()

public:
	UMultiverseClientComponent();

public:
	void Init();

	void Tick();

	void Deinit();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Host;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerPort;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ClientPort;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WorldName = TEXT("world");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SimulationName = TEXT("unreal");

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> SendObjects;

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> ReceiveObjects;

private:
	FMultiverseClient MultiverseClient;
};