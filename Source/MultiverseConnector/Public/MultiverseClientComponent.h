// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "CoreMinimal.h"
#include <string>
#include "MultiverseClientLibrary/multiverse_client.h"

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
class MULTIVERSECONNECTOR_API UMultiverseClientComponent final : public UObject, public MultiverseClient
{
	GENERATED_BODY()

public:
	UMultiverseClientComponent();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Host;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Port;

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> SendObjects;

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> ReceiveObjects;

private:
	TSharedPtr<FJsonObject> SendMetaDataJson;

	TSharedPtr<FJsonObject> ReceiveMetaDataJson;
	
	TArray<TPair<FString, EAttribute>> SendDataArray;

	TArray<TPair<FString, EAttribute>> ReceiveDataArray;

	FGraphEventRef ConnectToServerTask;

	FGraphEventRef MetaDataTask;

private:
	TMap<AActor *, FAttributeContainer> ReceiveObjectRefs;

	TMap<FString, AActor *> CachedActors;

	TMap<FString, TPair<class UMultiverseAnim *, FName>> CachedBoneNames;

	UPROPERTY(EditAnywhere)
	TMap<FLinearColor, FString> ColorMap;

private:
    bool compute_receive_meta_data() override;

    void compute_request_buffer_sizes(size_t &req_send_buffer_size, size_t &req_receive_buffer_size) const override;

    void compute_response_buffer_sizes(size_t &res_send_buffer_size, size_t &res_receive_buffer_size) const override;
	
    void start_connect_to_server_thread() override;

    void wait_for_connect_to_server_thread_finish() override;

    void start_meta_data_thread() override;

    void wait_for_meta_data_thread_finish() override;

    bool init_objects() override;

    void bind_send_meta_data() override;

    void bind_receive_meta_data() override;    

    void init_send_and_receive_data() override;

    void bind_send_data() override;

    void bind_receive_data() override;

    void clean_up() override;

private:
	UMaterial *GetMaterial(const FLinearColor &Color) const;
};