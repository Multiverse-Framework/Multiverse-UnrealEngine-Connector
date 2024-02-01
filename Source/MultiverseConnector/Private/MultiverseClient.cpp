// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultiverseClient.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "Json.h"
#include "Math/UnrealMathUtility.h"
#include "MultiverseAnim.h"
#include "MultiverseClient.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include <chrono>

DEFINE_LOG_CATEGORY_STATIC(LogMultiverseClientComponent, Log, All);

const TMap<EAttribute, TArray<double>> AttributeDataMap =
	{
		{EAttribute::Position, {0.0, 0.0, 0.0}},
		{EAttribute::Quaternion, {1.0, 0.0, 0.0, 0.0}},
		{EAttribute::JointRvalue, {0.0}},
		{EAttribute::JointTvalue, {0.0}},
		{EAttribute::JointPosition, {0.0, 0.0, 0.0}},
		{EAttribute::JointQuaternion, {1.0, 0.0, 0.0, 0.0}}};

const TMap<FString, EAttribute> AttributeStringMap =
	{
		{TEXT("position"), EAttribute::Position},
		{TEXT("quaternion"), EAttribute::Quaternion},
		{TEXT("joint_rvalue"), EAttribute::JointRvalue},
		{TEXT("joint_tvalue"), EAttribute::JointTvalue},
		{TEXT("joint_position"), EAttribute::JointPosition},
		{TEXT("joint_quaternion"), EAttribute::JointQuaternion}};

static void BindMetaData(const TSharedPtr<FJsonObject> &MetaDataJson,
						 const TPair<AActor *, FAttributeContainer> &Object,
						 TMap<FString, AActor *> &CachedActors,
						 TMap<FString, TPair<UMultiverseAnim *, FName>> &CachedBoneNames)
{
	if (AStaticMeshActor *StaticMeshActor = Cast<AStaticMeshActor>(Object.Key))
	{
		if (UStaticMeshComponent *StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent())
		{
			StaticMeshComponent->SetSimulatePhysics(false);
		}

		TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
		for (const EAttribute &Attribute : Object.Value.Attributes)
		{
			switch (Attribute)
			{
			case EAttribute::Position:
				AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("position"))));
				break;

			case EAttribute::Quaternion:
				AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("quaternion"))));
				break;

			default:
				break;
			}
		}

		FString ObjectName = Object.Key->GetName();
		CachedActors.Add(ObjectName, Object.Key);
		ObjectName.RemoveFromEnd(TEXT("_ref"));
		MetaDataJson->SetArrayField(ObjectName, AttributeJsonArray);
	}
	else if (ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(Object.Key))
	{

		TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
		for (const EAttribute &Attribute : Object.Value.Attributes)
		{
			switch (Attribute)
			{
			case EAttribute::Position:
				AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("position"))));
				break;

			case EAttribute::Quaternion:
				AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("quaternion"))));
				break;

			default:
				break;
			}
		}

		FString ObjectName = Object.Key->GetName();
		CachedActors.Add(ObjectName, Object.Key);
		MetaDataJson->SetArrayField(ObjectName, AttributeJsonArray);

		if (USkeletalMeshComponent *SkeletalMeshComponent = SkeletalMeshActor->GetSkeletalMeshComponent())
		{
			if (UMultiverseAnim *MultiverseAnim = Cast<UMultiverseAnim>(SkeletalMeshComponent->GetAnimInstance()))
			{
				TArray<FName> BoneNames;
				SkeletalMeshComponent->GetBoneNames(BoneNames);
				BoneNames.Sort([](const FName &BoneNameA, const FName &BoneNameB)
							   { return BoneNameB.ToString().Compare(BoneNameA.ToString()) > 0; });
				for (const FName &BoneName : BoneNames)
				{
					FString BoneNameStr = BoneName.ToString();
					if (((BoneNameStr.EndsWith(TEXT("_revolute_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_revolute_bone"))) ||
						 (BoneNameStr.EndsWith(TEXT("_continuous_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_continuous_bone")))) &&
						Object.Value.Attributes.Contains(EAttribute::JointRvalue))
					{
						AttributeJsonArray = {MakeShareable(new FJsonValueString(TEXT("joint_rvalue")))};
						CachedBoneNames.Add(BoneNameStr, TPair<UMultiverseAnim *, FName>(MultiverseAnim, BoneName));
						MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
					}
					else if ((BoneNameStr.EndsWith(TEXT("_prismatic_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_prismatic_bone"))) &&
							 Object.Value.Attributes.Contains(EAttribute::JointTvalue))
					{
						AttributeJsonArray = {MakeShareable(new FJsonValueString(TEXT("joint_tvalue")))};
						CachedBoneNames.Add(BoneNameStr, TPair<UMultiverseAnim *, FName>(MultiverseAnim, BoneName));
						MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
					}
				}
			}
			else
			{
				UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a MultiverseAnim."), *SkeletalMeshActor->GetName())
			}
		}
		else
		{
			UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a UStaticMeshComponent."), *SkeletalMeshActor->GetName())
		}
	}
}

static void BindDataArray(TArray<TPair<FString, EAttribute>> &DataArray,
						  const TPair<AActor *, FAttributeContainer> &Object)
{
	if (AStaticMeshActor *StaticMeshActor = Cast<AStaticMeshActor>(Object.Key))
	{
		TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
		for (const EAttribute &Attribute : Object.Value.Attributes)
		{
			DataArray.Add(TPair<FString, EAttribute>(Object.Key->GetName(), Attribute));
		}
	}
	else if (ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(Object.Key))
	{
		if (Object.Value.Attributes.Contains(EAttribute::Position))
		{
			DataArray.Add(TPair<FString, EAttribute>(Object.Key->GetName(), EAttribute::Position));
		}

		if (Object.Value.Attributes.Contains(EAttribute::Quaternion))
		{
			DataArray.Add(TPair<FString, EAttribute>(Object.Key->GetName(), EAttribute::Quaternion));
		}

		if (USkeletalMeshComponent *SkeletalMeshComponent = SkeletalMeshActor->GetSkeletalMeshComponent())
		{
			if (UMultiverseAnim *MultiverseAnim = Cast<UMultiverseAnim>(SkeletalMeshComponent->GetAnimInstance()))
			{
				TArray<FName> BoneNames;
				SkeletalMeshComponent->GetBoneNames(BoneNames);
				BoneNames.Sort([](const FName &BoneNameA, const FName &BoneNameB)
							   { return BoneNameB.ToString().Compare(BoneNameA.ToString()) > 0; });
				for (const FName &BoneName : BoneNames)
				{
					FString BoneNameStr = BoneName.ToString();
					if (((BoneNameStr.EndsWith(TEXT("_revolute_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_revolute_bone"))) ||
						 (BoneNameStr.EndsWith(TEXT("_continuous_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_continuous_bone")))) &&
						Object.Value.Attributes.Contains(EAttribute::JointRvalue))
					{
						DataArray.Add(TPair<FString, EAttribute>(BoneNameStr, EAttribute::JointRvalue));
					}
					else if ((BoneNameStr.EndsWith(TEXT("_prismatic_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_prismatic_bone"))) &&
							 Object.Value.Attributes.Contains(EAttribute::JointTvalue))
					{
						DataArray.Add(TPair<FString, EAttribute>(BoneNameStr, EAttribute::JointTvalue));
					}
				}
			}
			else
			{
				UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a MultiverseAnim."), *SkeletalMeshActor->GetName())
			}
		}
		else
		{
			UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a UStaticMeshComponent."), *SkeletalMeshActor->GetName())
		}

		DataArray.Sort([](const TPair<FString, EAttribute> &DataA, const TPair<FString, EAttribute> &DataB)
					   { return DataB.Key.Compare(DataA.Key) > 0; });
	}
}

FMultiverseClient::FMultiverseClient()
{
	ColorMap = {
		{FLinearColor(0, 0, 1, 1), TEXT("Blue")},
		{FLinearColor(0, 1, 1, 1), TEXT("Cyan")},
		{FLinearColor(0, 1, 0, 1), TEXT("Green")},
		{FLinearColor(1, 0, 0.5, 1), TEXT("Pink")},
		{FLinearColor(0.5, 0, 1, 1), TEXT("Purple")},
		{FLinearColor(1, 0, 0, 1), TEXT("Red")},
		{FLinearColor(1, 1, 0, 1), TEXT("Yellow")},
		{FLinearColor(0.8, 0.1, 0, 1), TEXT("Orange")},
		{FLinearColor(0.1, 0.1, 0.1, 1), TEXT("Gray")}};
}

void FMultiverseClient::Init(const FString &Host, const FString &ServerPort, const FString &ClientPort,
							 const FString &InWorldName, const FString &InSimulationName,
							 TMap<AActor *, FAttributeContainer> &InSendObjects,
							 TMap<AActor *, FAttributeContainer> &InReceiveObjects,
							 UWorld *InWorld)
{
	SendObjects = InSendObjects;
	ReceiveObjects = InReceiveObjects;
	World = InWorld;
	WorldName = InWorldName;
	SimulationName = InSimulationName;

	host = TCHAR_TO_UTF8(*Host);
	port = TCHAR_TO_UTF8(*ClientPort);

	server_socket_addr = host + ":" + TCHAR_TO_UTF8(*ServerPort);

	connect();
}

UMaterial *FMultiverseClient::GetMaterial(const FLinearColor &Color) const
{
	const FString ColorName = TEXT("M_") + ColorMap[Color];
	return Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *(TEXT("Material'/MultiverseConnector/Assets/Materials/") + ColorName + TEXT(".") + ColorName + TEXT("'"))));
}

bool FMultiverseClient::compute_response_meta_data()
{
	ResponseMetaDataJson = MakeShareable(new FJsonObject);
	if (response_meta_data_str.empty())
	{
		return false;
	}

	FString ResponseMetaDataString(response_meta_data_str.c_str());
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseMetaDataString);

	return FJsonSerializer::Deserialize(Reader, ResponseMetaDataJson) &&
		   ResponseMetaDataJson->HasField("time") &&
		   ResponseMetaDataJson->GetNumberField("time") > 0;
}

void FMultiverseClient::compute_request_buffer_sizes(size_t &req_send_buffer_size, size_t &req_receive_buffer_size) const
{
	TMap<FString, size_t> RequestBufferSizes = {{TEXT("send"), 1}, {TEXT("receive"), 1}};

	for (TPair<FString, size_t> &RequestBufferSize : RequestBufferSizes)
	{
		if (!RequestMetaDataJson->HasField(RequestBufferSize.Key))
		{
			break;
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>> &SendObjectJson : RequestMetaDataJson->GetObjectField(RequestBufferSize.Key)->Values)
		{
			if (SendObjectJson.Key.Compare(TEXT("")) == 0)
			{
				RequestBufferSize.Value = -1;
				break;
			}

			for (const TSharedPtr<FJsonValue> &SendObjectAttribute : SendObjectJson.Value->AsArray())
			{
				if (SendObjectAttribute->AsString().Compare(TEXT("")) == 0)
				{
					RequestBufferSize.Value = -1;
					break;
				}

				if (AttributeStringMap.Contains(SendObjectAttribute->AsString()))
				{
					RequestBufferSize.Value += AttributeDataMap[AttributeStringMap[SendObjectAttribute->AsString()]].Num();
				}
			}
		}
	}

	req_send_buffer_size = RequestBufferSizes[TEXT("send")];
	req_receive_buffer_size = RequestBufferSizes[TEXT("receive")];
}

void FMultiverseClient::compute_response_buffer_sizes(size_t &res_send_buffer_size, size_t &res_receive_buffer_size) const
{
	TMap<FString, size_t> ResponseBufferSizes = {{TEXT("send"), 1}, {TEXT("receive"), 1}};

	for (TPair<FString, size_t> &ResponseBufferSize : ResponseBufferSizes)
	{
		if (!ResponseMetaDataJson->HasField(ResponseBufferSize.Key))
		{
			continue;
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>> &ReceiveObject : ResponseMetaDataJson->GetObjectField(ResponseBufferSize.Key)->Values)
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>> &ReceiveObjectData : ReceiveObject.Value->AsObject()->Values)
			{
				ResponseBufferSize.Value += ReceiveObjectData.Value->AsArray().Num();
			}
		}
	}

	res_send_buffer_size = ResponseBufferSizes[TEXT("send")];
	res_receive_buffer_size = ResponseBufferSizes[TEXT("receive")];
}

bool FMultiverseClient::init_objects(bool from_server)
{
	SendObjects.Remove(nullptr);
	ReceiveObjects.Remove(nullptr);
	
	if (SendObjects.Num() > 0)
	{
		SendObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
							{	return ActorB.GetName().Compare(ActorA.GetName()) > 0; });
	}
	if (ReceiveObjects.Num() > 0)
	{
		ReceiveObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
								{	return ActorB.GetName().Compare(ActorA.GetName()) > 0; });
	}

	if (World == nullptr)
	{
		UE_LOG(LogMultiverseClientComponent, Error, TEXT("World is nullptr"))
		return false;
	}

	for (const TPair<AActor *, FAttributeContainer> &ReceiveObject : ReceiveObjects)
	{
		if (ReceiveObject.Key == nullptr)
		{
			UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in ReceiveObjects."))
			continue;
		}

		if (AStaticMeshActor *StaticMeshActor = Cast<AStaticMeshActor>(ReceiveObject.Key))
		{
			UStaticMeshComponent *StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
			if (StaticMeshComponent == nullptr || StaticMeshComponent->GetStaticMesh() == nullptr)
			{
				UE_LOG(LogMultiverseClientComponent, Warning, TEXT("StaticMeshActor %s in ReceiveObjects has None StaticMeshComponent."), *ReceiveObject.Key->GetName())
				continue;
			}
			if (!StaticMeshComponent->IsSimulatingPhysics())
			{
				UE_LOG(LogMultiverseClientComponent, Warning, TEXT("StaticMeshActor %s has disabled physics, enabling physics."), *ReceiveObject.Key->GetName())
				StaticMeshComponent->SetSimulatePhysics(true);
			}

			UMaterial *RedMaterial = GetMaterial(FLinearColor(1, 0, 0, 1));
			for (int32 i = 0; i < StaticMeshComponent->GetMaterials().Num(); i++)
			{
				StaticMeshComponent->SetMaterial(i, RedMaterial);
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.Template = StaticMeshActor;
			SpawnParams.Name = *(ReceiveObject.Key->GetName() + TEXT("_ref"));
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor *ReceiveObjectRef = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(), SpawnParams);
			if (ReceiveObjectRef == nullptr)
			{
				UE_LOG(LogMultiverseClientComponent, Error, TEXT("Failed to spawn StaticMeshActor %s"), *SpawnParams.Name.ToString())
				continue;
			}

#if WITH_EDITOR
			ReceiveObjectRef->SetActorLabel(SpawnParams.Name.ToString());
#endif

			AStaticMeshActor *StaticMeshActorRef = Cast<AStaticMeshActor>(ReceiveObjectRef);

			UStaticMeshComponent *StaticMeshComponentRef = StaticMeshActorRef->GetStaticMeshComponent();
			StaticMeshComponentRef->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			StaticMeshComponentRef->SetSimulatePhysics(false);

			UMaterial *GrayMaterial = GetMaterial(FLinearColor(0.1, 0.1, 0.1, 1));
			for (int32 i = 0; i < StaticMeshComponentRef->GetMaterials().Num(); i++)
			{
				StaticMeshComponentRef->SetMaterial(i, GrayMaterial);
			}

			UPhysicsConstraintComponent *PhysicsConstraint = NewObject<UPhysicsConstraintComponent>(StaticMeshActorRef);
			PhysicsConstraint->AttachToComponent(StaticMeshComponentRef, FAttachmentTransformRules::KeepWorldTransform);
			PhysicsConstraint->SetWorldLocation(StaticMeshActorRef->GetActorLocation());

			PhysicsConstraint->ComponentName1.ComponentName = *StaticMeshActorRef->GetName();
			PhysicsConstraint->ComponentName2.ComponentName = *StaticMeshActor->GetName();
			PhysicsConstraint->ConstraintActor1 = StaticMeshActorRef;
			PhysicsConstraint->ConstraintActor2 = StaticMeshActor;
			PhysicsConstraint->ConstraintInstance.Pos1 = FVector();
			PhysicsConstraint->ConstraintInstance.Pos2 = FVector();

			PhysicsConstraint->SetConstrainedComponents(StaticMeshComponentRef, NAME_None, StaticMeshComponent, NAME_None);

			// Enable linear constraint in all axes
			PhysicsConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
			PhysicsConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
			PhysicsConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
			// PhysicsConstraint->ConstraintInstance.ProfileInstance.LinearLimit.bSoftConstraint = true;

			// Enable angular constraint in all axes
			PhysicsConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
			PhysicsConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
			PhysicsConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.f);
			// PhysicsConstraint->ConstraintInstance.ProfileInstance.ConeLimit.bSoftConstraint = true;
			// PhysicsConstraint->ConstraintInstance.ProfileInstance.TwistLimit.bSoftConstraint = true;

			PhysicsConstraint->RegisterComponent();

			ReceiveObjectRefs.Add(ReceiveObjectRef, ReceiveObject.Value);
		}
		else if (ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(ReceiveObject.Key))
		{
			ReceiveObjectRefs.Add(ReceiveObject.Key, ReceiveObject.Value);
		}
	}

	return SendObjects.Num() > 0 || ReceiveObjectRefs.Num() > 0;
}

void FMultiverseClient::start_connect_to_server_thread()
{
	ConnectToServerTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
																		 { FMultiverseClient::connect_to_server(); },
																		 TStatId(), nullptr, ENamedThreads::AnyThread);
}

void FMultiverseClient::wait_for_connect_to_server_thread_finish()
{
	if (ConnectToServerTask.IsValid())
	{
		ConnectToServerTask->Wait();
	}
}

void FMultiverseClient::start_meta_data_thread()
{
	MetaDataTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
																  { FMultiverseClient::send_and_receive_meta_data(); },
																  TStatId(), nullptr, ENamedThreads::AnyThread);
}

void FMultiverseClient::wait_for_meta_data_thread_finish()
{
	if (MetaDataTask.IsValid())
	{
		MetaDataTask->Wait();
	}
}

void FMultiverseClient::bind_request_meta_data()
{
	RequestMetaDataJson = MakeShareable(new FJsonObject);
	RequestMetaDataJson->SetStringField(TEXT("world_name"), WorldName);
	RequestMetaDataJson->SetStringField(TEXT("simulation_name"), SimulationName);
	RequestMetaDataJson->SetStringField(TEXT("time_unit"), TEXT("s"));
	RequestMetaDataJson->SetStringField(TEXT("length_unit"), TEXT("cm"));
	RequestMetaDataJson->SetStringField(TEXT("angle_unit"), TEXT("deg"));
	RequestMetaDataJson->SetStringField(TEXT("handedness"), TEXT("lhs"));
	RequestMetaDataJson->SetStringField(TEXT("force_unit"), TEXT("N"));

	RequestMetaDataJson->SetObjectField(TEXT("send"), MakeShareable(new FJsonObject));
	RequestMetaDataJson->SetObjectField(TEXT("receive"), MakeShareable(new FJsonObject));

	for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
	{
		if (SendObject.Key == nullptr)
		{
			UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in SendObjects"))
			continue;
		}

		BindMetaData(RequestMetaDataJson->GetObjectField(TEXT("send")), SendObject, CachedActors, CachedBoneNames);
	}

	for (const TPair<AActor *, FAttributeContainer> &ReceiveObjectRef : ReceiveObjectRefs)
	{
		if (ReceiveObjectRef.Key == nullptr)
		{
			UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in ReceiveObjectRefs"))
			continue;
		}

		BindMetaData(RequestMetaDataJson->GetObjectField(TEXT("receive")), ReceiveObjectRef, CachedActors, CachedBoneNames);
	}

	FString RequestMetaDataString;
	TSharedRef<TJsonWriter<TCHAR>> Writer = TJsonWriterFactory<TCHAR>::Create(&RequestMetaDataString);
	FJsonSerializer::Serialize(RequestMetaDataJson.ToSharedRef(), Writer, true);

	request_meta_data_str.clear();
	for (size_t i = 0; i < (RequestMetaDataString.Len() + 127) / 128; i++) // Split string into multiple substrings with a length of 128 characters or less
	{
		const int32 StartIndex = i * 128;
		const int32 SubstringLength = FMath::Min(128, RequestMetaDataString.Len() - StartIndex);
		const FString Substring = RequestMetaDataString.Mid(StartIndex, SubstringLength);
		request_meta_data_str += StringCast<ANSICHAR>(*Substring).Get();
	}

	UE_LOG(LogMultiverseClientComponent, Log, TEXT("%s"), *RequestMetaDataString)
}

void FMultiverseClient::bind_response_meta_data()
{
	if (!ResponseMetaDataJson->HasField(TEXT("send")))
	{
		return;
	}

	TSharedPtr<FJsonObject> ResponseSendObjects = ResponseMetaDataJson->GetObjectField(TEXT("send"));

	for (const TPair<FString, EAttribute> &SendData : SendDataArray)
	{
		if (CachedActors.Contains(SendData.Key))
		{
			if (CachedActors[SendData.Key] == nullptr)
			{
				UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in CachedActors"))
				continue;
			}

			switch (SendData.Value)
			{
			case EAttribute::Position:
			{
				if (!ResponseSendObjects->HasField("position"))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> ObjectPosition = ResponseSendObjects->GetArrayField(TEXT("position"));
				if (ObjectPosition.Num() != 3)
				{
					continue;
				}

				const double X = ObjectPosition[0]->AsNumber();
				const double Y = ObjectPosition[1]->AsNumber();
				const double Z = ObjectPosition[2]->AsNumber();
				CachedActors[SendData.Key]->SetActorLocation(FVector(X, Y, Z));
				break;
			}

			case EAttribute::Quaternion:
			{
				if (!ResponseSendObjects->HasField(TEXT("quaternion")))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> ObjectQuaternion = ResponseSendObjects->GetArrayField(TEXT("quaternion"));
				if (ObjectQuaternion.Num() != 4)
				{
					continue;
				}

				const double W = ObjectQuaternion[0]->AsNumber();
				const double X = ObjectQuaternion[1]->AsNumber();
				const double Y = ObjectQuaternion[2]->AsNumber();
				const double Z = ObjectQuaternion[3]->AsNumber();
				CachedActors[SendData.Key]->SetActorRotation(FQuat(X, Y, Z, W));
				break;
			}

			default:
				break;
			}
		}
		else if (CachedBoneNames.Contains(SendData.Key))
		{
			switch (SendData.Value)
			{
			case EAttribute::JointRvalue:
			{
				if (!ResponseSendObjects->HasField(TEXT("joint_rvalue")))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> JointRvalue = ResponseSendObjects->GetArrayField(TEXT("joint_rvalue"));
				if (JointRvalue.Num() != 1)
				{
					continue;
				}
				CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].SetRotation(FQuat(FRotator(JointRvalue[0]->AsNumber(), 0.f, 0.f)));
				break;
			}

			case EAttribute::JointTvalue:
			{
				if (!ResponseSendObjects->HasField(TEXT("joint_tvalue")))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> JointTvalue = ResponseSendObjects->GetArrayField(TEXT("joint_tvalue"));
				if (JointTvalue.Num() != 1)
				{
					continue;
				}

				CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].SetTranslation(FVector(0.f, JointTvalue[0]->AsNumber(), 0.f));
				break;
			}

			default:
				break;
			}
		}
	}
}

void FMultiverseClient::init_send_and_receive_data()
{
	for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
	{
		if (SendObject.Key == nullptr)
		{
			UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in SendObjects"))
			continue;
		}

		BindDataArray(SendDataArray, SendObject);
	}

	for (const TPair<AActor *, FAttributeContainer> &ReceiveObjectRef : ReceiveObjectRefs)
	{
		if (ReceiveObjectRef.Key == nullptr)
		{
			UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in ReceiveObjectRefs"))
			continue;
		}

		BindDataArray(ReceiveDataArray, ReceiveObjectRef);
	}
}

void FMultiverseClient::bind_send_data()
{
	double *send_buffer_addr = send_buffer + 1;

	for (const TPair<FString, EAttribute> &SendData : SendDataArray)
	{
		if (CachedActors.Contains(SendData.Key))
		{
			if (CachedActors[SendData.Key] == nullptr)
			{
				UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in CachedActors"))
				continue;
			}

			switch (SendData.Value)
			{
			case EAttribute::Position:
			{
				const FVector ActorLocation = CachedActors[SendData.Key]->GetActorLocation();
				*send_buffer_addr++ = ActorLocation.X;
				*send_buffer_addr++ = ActorLocation.Y;
				*send_buffer_addr++ = ActorLocation.Z;
				break;
			}

			case EAttribute::Quaternion:
			{
				const FQuat ActorQuat = CachedActors[SendData.Key]->GetActorQuat();
				*send_buffer_addr++ = ActorQuat.W;
				*send_buffer_addr++ = ActorQuat.X;
				*send_buffer_addr++ = ActorQuat.Y;
				*send_buffer_addr++ = ActorQuat.Z;
				break;
			}

			default:
				break;
			}
		}
		else if (CachedBoneNames.Contains(SendData.Key))
		{
			switch (SendData.Value)
			{
			case EAttribute::JointRvalue:
			{
				const FQuat JointQuaternion = CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].GetRotation();
				*send_buffer_addr++ = FMath::RadiansToDegrees(JointQuaternion.GetAngle());
				break;
			}

			case EAttribute::JointTvalue:
			{
				const FVector JointPosition = CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].GetTranslation();
				*send_buffer_addr++ = JointPosition.Y;
				break;
			}

			default:
				break;
			}
		}
	}
}

void FMultiverseClient::bind_receive_data()
{
	double *receive_buffer_addr = receive_buffer + 1;

	for (const TPair<FString, EAttribute> &ReceiveData : ReceiveDataArray)
	{
		if (CachedActors.Contains(ReceiveData.Key))
		{
			if (CachedActors[ReceiveData.Key] == nullptr)
			{
				UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in CachedActors"))
				continue;
			}

			switch (ReceiveData.Value)
			{
			case EAttribute::Position:
			{
				const double X = *receive_buffer_addr++;
				const double Y = *receive_buffer_addr++;
				const double Z = *receive_buffer_addr++;
				CachedActors[ReceiveData.Key]->SetActorLocation(FVector(X, Y, Z));
				break;
			}

			case EAttribute::Quaternion:
			{
				const double W = *receive_buffer_addr++;
				const double X = *receive_buffer_addr++;
				const double Y = *receive_buffer_addr++;
				const double Z = *receive_buffer_addr++;
				CachedActors[ReceiveData.Key]->SetActorRotation(FQuat(X, Y, Z, W));
				break;
			}

			default:
				break;
			}
		}
		else if (CachedBoneNames.Contains(ReceiveData.Key))
		{
			switch (ReceiveData.Value)
			{
			case EAttribute::JointRvalue:
			{
				const double JointRvalue = *receive_buffer_addr++;
				CachedBoneNames[ReceiveData.Key].Key->JointPoses[CachedBoneNames[ReceiveData.Key].Value].SetRotation(FQuat(FRotator(JointRvalue, 0.f, 0.f)));
				break;
			}

			case EAttribute::JointTvalue:
			{
				const double JointTvalue = *receive_buffer_addr++;
				CachedBoneNames[ReceiveData.Key].Key->JointPoses[CachedBoneNames[ReceiveData.Key].Value].SetTranslation(FVector(0.f, JointTvalue, 0.f));
				break;
			}

			default:
				break;
			}
		}
	}
}

void FMultiverseClient::clean_up()
{
	SendDataArray.Empty();

	ReceiveDataArray.Empty();
}

void FMultiverseClient::reset()
{
	
}