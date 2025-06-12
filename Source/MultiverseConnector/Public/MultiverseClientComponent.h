// Copyright (c) 2023, Giang Hoang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "CoreMinimal.h"
#include "MultiverseClient.h"

// clang-format off
#include "MultiverseClientComponent.generated.h"
// clang-format on

struct FAttributeContainer;

struct FApiCallbacks;

UCLASS(Blueprintable, DefaultToInstanced, collapsecategories, hidecategories = Object, editinlinenew)
class MULTIVERSECONNECTOR_API UMultiverseClientComponent final : public UObject
{
	GENERATED_BODY()

public:
	UMultiverseClientComponent();

public:
	void Init();

	void Tick(float DeltaTime);

	void Deinit();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerHost = TEXT("tcp://127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerPort = TEXT("7000");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ClientPort = TEXT("9000");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WorldName = TEXT("world");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SimulationName = TEXT("unreal");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UpdateRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoSendHandsAndHead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<AActor*, FAttributeContainer> SendObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<AActor*, FAttributeContainer> ReceiveObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FAttributeDataContainer> SendCustomObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FAttributeDataContainer> ReceiveCustomObjects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API Callbacks")
	bool bSimulationApiCallbacksEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API Callbacks")
	float SimulationApiCallbacksRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API Callbacks")
	TMap<FString, FApiCallbacks> SimulationApiCallbacks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "API Callbacks")
	TMap<FString, FApiCallbacks> SimulationApiCallbacksResponse;

private:
	FMultiverseClient MultiverseClient;

	float CurrentCycleTime = 0.f;

	float CurrentSimulationApiCycleTime = 0.f;
};
