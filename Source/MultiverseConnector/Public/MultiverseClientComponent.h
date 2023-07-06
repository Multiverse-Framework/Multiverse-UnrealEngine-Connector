// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "CoreMinimal.h"
#include <string>

// clang-format off
#include "MultiverseClientComponent.generated.h"
// clang-format on

UENUM()
enum class EAttribute : uint8
{
	Position,
	Quaternion,
	JointRvalue,
	JointTvalue,
	JointPosition,
	JointQuaternion
};

USTRUCT(Blueprintable)
struct FAttributeContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSet<EAttribute> Attributes;
};

class ASkeletalMeshActor;

UCLASS(Blueprintable, DefaultToInstanced, collapsecategories, hidecategories = Object, editinlinenew)
class MULTIVERSECONNECTOR_API UMultiverseClientComponent : public UObject
{
	GENERATED_BODY()

public:
	UMultiverseClientComponent();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Host;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Port;

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> SendObjects;

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> ReceiveObjects;

private:
	TArray<TPair<FString, EAttribute>> SendDataArray;

	TArray<TPair<FString, EAttribute>> ReceiveDataArray;

	void *context;

	void *socket_client;

	size_t send_buffer_size = 1;

	size_t receive_buffer_size = 1;

	double *send_buffer;

	double *receive_buffer;

	FString SocketAddr;

	std::string socket_addr;

	TMap<AActor *, FAttributeContainer> ReceiveObjectRefs;

	FGraphEventRef Task;

	TMap<FString, AActor *> CachedActors;

	TMap<FString, TPair<class UMultiverseAnim *, FName>> CachedBoneNames;

private:
	UPROPERTY(VisibleAnywhere)
	bool IsEnable = false;

	UPROPERTY(EditAnywhere)
	TMap<FLinearColor, FString> ColorMap;

public:
	void Init();

	void SendMetaData();

	void Deinit();

	void Communicate();

private:
	UMaterial *GetMaterial(const FLinearColor &Color) const;
};