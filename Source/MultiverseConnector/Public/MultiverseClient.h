// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
THIRD_PARTY_INCLUDES_START
#include "MultiverseClientLibrary/multiverse_client.h"
THIRD_PARTY_INCLUDES_END
#include "MultiverseClient.generated.h"

UENUM()
enum class EAttribute : uint8
{
	JointPosition,
	JointQuaternion,
	JointRvalue,
	JointTvalue,
	Position,
	Quaternion
};

USTRUCT(Blueprintable)
struct FAttributeContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ObjectName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttribute> Attributes;
};

class MULTIVERSECONNECTOR_API FMultiverseClient final : public MultiverseClient
{
public:
	FMultiverseClient();

public:
	void Init(const FString& ServerHost, const FString& ServerPort,
	          const FString& ClientHost, const FString& ClientPort,
	          const FString& InWorldName, const FString& InSimulationName,
	          const TMap<AActor*, FAttributeContainer>& InSendObjects,
	          const TMap<AActor*, FAttributeContainer>& InReceiveObjects,
	          UWorld* InWorld);

private:
	TMap<AActor*, FAttributeContainer> SendObjects;

	TMap<AActor*, FAttributeContainer> ReceiveObjects;

	TSharedPtr<FJsonObject> RequestMetaDataJson;

	TSharedPtr<FJsonObject> ResponseMetaDataJson;

	TArray<TPair<FString, EAttribute>> SendDataArray;

	TArray<TPair<FString, EAttribute>> ReceiveDataArray;

	FGraphEventRef ConnectToServerTask;

	FGraphEventRef MetaDataTask;

private:
	UWorld* World = nullptr;

	FString WorldName;

	FString SimulationName;

	TMap<AActor*, FAttributeContainer> ReceiveObjectRefs;

	TMap<FString, AActor*> CachedActors;

	TMap<FString, TPair<class UMultiverseAnim*, FName>> CachedBoneNames;

	TMap<FLinearColor, FString> ColorMap;

private:
	virtual bool compute_response_meta_data() override;

	virtual void
	compute_request_buffer_sizes(size_t& req_send_buffer_size, size_t& req_receive_buffer_size) const override;

	virtual void
	compute_response_buffer_sizes(size_t& res_send_buffer_size, size_t& res_receive_buffer_size) const override;

	virtual void start_connect_to_server_thread() override;

	virtual void wait_for_connect_to_server_thread_finish() override;

	virtual void start_meta_data_thread() override;

	virtual void wait_for_meta_data_thread_finish() override;

	virtual bool init_objects(bool from_server = false) override;

	virtual void bind_request_meta_data() override;

	virtual void bind_response_meta_data() override;

	virtual void init_send_and_receive_data() override;

	virtual void bind_send_data() override;

	virtual void bind_receive_data() override;

	virtual void clean_up() override;

	virtual void reset() override;

private:
	UMaterial* GetMaterial(const FLinearColor& Color) const;
};
