// Copyright Epic Games Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
THIRD_PARTY_INCLUDES_START
#include "ThirdParty/MultiverseClientLibrary/multiverse_client.h"
THIRD_PARTY_INCLUDES_END
#include "MultiverseClient.generated.h"

UENUM()
enum class EAttribute : uint8
{
	CmdJointTorque,
	CmdJointForce,
	CmdJointAngularAcceleration,
	CmdJointLinearAcceleration,
	CmdJointAngularVelocity,
	CmdJointLinearVelocity,
	CmdJointRvalue,
	CmdJointTvalue,
	Depth_1280_1024,
	Depth_128_128,
	Depth_3840_2160,
	Depth_640_480,
	JointAngularAcceleration,
	JointLinearAcceleration,
	JointAngularVelocity,
	JointLinearVelocity,
	JointPosition,
	JointQuaternion,
	JointRvalue,
	JointTvalue,
	Position,
	Quaternion,
	RGB_1280_1024,
	RGB_128_128,
	RGB_3840_2160,
	RGB_640_480,
};

USTRUCT(Blueprintable)
struct FAttributeContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ObjectName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ObjectPrefix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ObjectSuffix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EAttribute> Attributes;
};

USTRUCT(Blueprintable)
struct FDataContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<double> Data;
};

USTRUCT(Blueprintable)
struct FAttributeDataContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EAttribute, FDataContainer> Attributes;
};

USTRUCT(Blueprintable)
struct FApiCallback
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> Arguments;
};

USTRUCT(Blueprintable)
struct FApiCallbacks
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FApiCallback> ApiCallbacks;
};

class MULTIVERSECONNECTOR_API FMultiverseClient : public MultiverseClient
{
public:
	FMultiverseClient();

public:
	void Init(const FString &ServerHost, const FString &ServerPort, const FString &ClientPort,
			  const FString &WorldName, const FString &SimulationName,
			  TMap<AActor *, FAttributeContainer> &InSendObjects,
			  TMap<AActor *, FAttributeContainer> &InReceiveObjects,
			  TMap<FString, FAttributeDataContainer> *InSendCustomObjectsPtr,
			  TMap<FString, FAttributeDataContainer> *InReceiveCustomObjectsPtr,
			  UWorld *World);

	TMap<FString, FApiCallbacks> CallApis(const TMap<FString, FApiCallbacks> &SimulationApiCallbacks);

private:
	TMap<AActor *, FAttributeContainer> SendObjects;

	TMap<AActor *, FAttributeContainer> ReceiveObjects;

	TMap<FString, FAttributeDataContainer> *SendCustomObjectsPtr;

	TMap<FString, FAttributeDataContainer> *ReceiveCustomObjectsPtr;

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

	TMap<FString, UActorComponent *> CachedComponents;

	TMap<FString, TMap<class UMultiverseAnim *, FName>> CachedBoneNames;

	TMap<FLinearColor, FString> ColorMap;

	float StartTime = -1.f;

	bool bComputingRequestAndResponseMetaData = false;

private:
	void start_connect_to_server_thread() override;

	void wait_for_connect_to_server_thread_finish() override;

	void start_meta_data_thread() override;

	void wait_for_meta_data_thread_finish() override;

	bool init_objects(bool from_request_meta_data = false) override;

	void bind_request_meta_data() override;

	bool compute_request_and_response_meta_data() override final;

	void compute_request_buffer_sizes(std::map<std::string, size_t> &send_buffer_size, std::map<std::string, size_t> &receive_buffer_size) const override final;

    void compute_response_buffer_sizes(std::map<std::string, size_t> &send_buffer_size, std::map<std::string, size_t> &receive_buffer_size) const override final;

	void bind_api_callbacks() override;

	void bind_api_callbacks_response() override;

	void bind_response_meta_data() override;

	void init_send_and_receive_data() override;

	void bind_send_data() override;

	void bind_receive_data() override;

	void clean_up() override;

	void reset() override;

private:
	UMaterial *GetMaterial(const FLinearColor &Color) const;
};