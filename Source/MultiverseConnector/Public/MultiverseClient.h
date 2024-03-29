// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
THIRD_PARTY_INCLUDES_START
#include "ThirdParty/MultiverseClientLibrary/multiverse_client.h"
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

class MULTIVERSECONNECTOR_API FMultiverseClient : public MultiverseClient
{
public:
	FMultiverseClient();

public:
	void Init(const FString &ServerHost, const FString &ServerPort, const FString &ClientPort,
			  const FString &WorldName, const FString &SimulationName,
			  TMap<AActor *, FAttributeContainer> &SendObjects,
			  TMap<AActor *, FAttributeContainer> &ReceiveObjects,
			  UWorld *World);

private:
	TMap<AActor *, FAttributeContainer> SendObjects;

	TMap<AActor *, FAttributeContainer> ReceiveObjects;

	TSharedPtr<FJsonObject> RequestMetaDataJson;

	TSharedPtr<FJsonObject> ResponseMetaDataJson;

	TArray<TPair<FString, EAttribute>> SendDataArray;

	TArray<TPair<FString, EAttribute>> ReceiveDataArray;

	FGraphEventRef ConnectToServerTask;

	FGraphEventRef MetaDataTask;

private:
	UWorld *World;

	FString WorldName;

	FString SimulationName;

	TMap<FString, AActor *> CachedActors;

	TMap<FString, TPair<class UMultiverseAnim *, FName>> CachedBoneNames;

	TMap<FLinearColor, FString> ColorMap;

private:
	bool compute_response_meta_data() override;

	void compute_request_buffer_sizes(size_t &req_send_buffer_size, size_t &req_receive_buffer_size) const override;

	void compute_response_buffer_sizes(size_t &res_send_buffer_size, size_t &res_receive_buffer_size) const override;

	void start_connect_to_server_thread() override;

	void wait_for_connect_to_server_thread_finish() override;

	void start_meta_data_thread() override;

	void wait_for_meta_data_thread_finish() override;

	bool init_objects(bool from_server = false) override;

	void bind_request_meta_data() override;

	void bind_response_meta_data() override;

	void init_send_and_receive_data() override;

	void bind_send_data() override;

	void bind_receive_data() override;

	void clean_up() override;

	void reset() override;

private:
	UMaterial *GetMaterial(const FLinearColor &Color) const;
};