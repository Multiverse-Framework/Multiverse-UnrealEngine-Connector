// Copyright Epic Games, Inc. All Rights Reserved.

#include "MultiverseClient.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "Json.h"
#include "Math/UnrealMathUtility.h"
#include "MultiverseAnim.h"
#include "MultiverseClient.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Camera/CameraComponent.h"
#ifdef WIN32
#include "OculusXRHandComponent.h"
#include "UObject/Class.h"
#include "Camera/CameraComponent.h"
#endif
#include "Kismet/GameplayStatics.h"
#include <chrono>

DEFINE_LOG_CATEGORY_STATIC(LogMultiverseClient, Log, All);

TMap<EAttribute, TArray<double>> AttributeDoubleDataMap =
	{
		{EAttribute::Position, {0.0, 0.0, 0.0}},
		{EAttribute::Quaternion, {1.0, 0.0, 0.0, 0.0}},
		{EAttribute::JointRvalue, {0.0}},
		{EAttribute::JointTvalue, {0.0}},
		{EAttribute::JointAngularVelocity, {0.0}},
		{EAttribute::JointLinearVelocity, {0.0}},
		{EAttribute::JointAngularAcceleration, {0.0}},
		{EAttribute::JointLinearAcceleration, {0.0}},
		{EAttribute::CmdJointRvalue, {0.0}},
		{EAttribute::CmdJointTvalue, {0.0}},
		{EAttribute::CmdJointAngularVelocity, {0.0}},
		{EAttribute::CmdJointLinearVelocity, {0.0}},
		{EAttribute::CmdJointAngularAcceleration, {0.0}},
		{EAttribute::CmdJointLinearAcceleration, {0.0}},
		{EAttribute::CmdJointTorque, {0.0}},
		{EAttribute::CmdJointForce, {0.0}},
		{EAttribute::JointPosition, {0.0, 0.0, 0.0}},
		{EAttribute::JointQuaternion, {1.0, 0.0, 0.0, 0.0}}};

TMap<EAttribute, TArray<uint8_t>> AttributeUint8DataMap =
	{
		{EAttribute::RGB_3840_2160, TArray<uint8_t>()},
		{EAttribute::RGB_1280_1024, TArray<uint8_t>()},
		{EAttribute::RGB_640_480, TArray<uint8_t>()},
		{EAttribute::RGB_128_128, TArray<uint8_t>()}};

TMap<EAttribute, TArray<uint16_t>> AttributeUint16DataMap =
	{
		{EAttribute::Depth_3840_2160, TArray<uint16_t>()},
		{EAttribute::Depth_1280_1024, TArray<uint16_t>()},
		{EAttribute::Depth_640_480, TArray<uint16_t>()},
		{EAttribute::Depth_128_128, TArray<uint16_t>()}};

const TMap<FString, EAttribute> AttributeStringMap =
	{
		{TEXT("position"), EAttribute::Position},
		{TEXT("quaternion"), EAttribute::Quaternion},
		{TEXT("joint_rvalue"), EAttribute::JointRvalue},
		{TEXT("joint_tvalue"), EAttribute::JointTvalue},
		{TEXT("joint_angular_velocity"), EAttribute::JointAngularVelocity},
		{TEXT("joint_linear_velocity"), EAttribute::JointLinearVelocity},
		{TEXT("joint_angular_acceleration"), EAttribute::JointAngularAcceleration},
		{TEXT("joint_linear_acceleration"), EAttribute::JointLinearAcceleration},
		{TEXT("cmd_joint_rvalue"), EAttribute::CmdJointRvalue},
		{TEXT("cmd_joint_tvalue"), EAttribute::CmdJointTvalue},
		{TEXT("cmd_joint_angular_velocity"), EAttribute::CmdJointAngularVelocity},
		{TEXT("cmd_joint_linear_velocity"), EAttribute::CmdJointLinearVelocity},
		{TEXT("cmd_joint_angular_acceleration"), EAttribute::CmdJointAngularAcceleration},
		{TEXT("cmd_joint_linear_acceleration"), EAttribute::CmdJointLinearAcceleration},
		{TEXT("cmd_joint_torque"), EAttribute::CmdJointTorque},
		{TEXT("cmd_joint_force"), EAttribute::CmdJointForce},
		{TEXT("joint_position"), EAttribute::JointPosition},
		{TEXT("joint_quaternion"), EAttribute::JointQuaternion},
		{TEXT("rgb_3840_2160"), EAttribute::RGB_3840_2160},
		{TEXT("rgb_1280_1024"), EAttribute::RGB_1280_1024},
		{TEXT("rgb_640_480"), EAttribute::RGB_640_480},
		{TEXT("rgb_128_128"), EAttribute::RGB_128_128},
		{TEXT("depth_3840_2160"), EAttribute::Depth_3840_2160},
		{TEXT("depth_1280_1024"), EAttribute::Depth_1280_1024},
		{TEXT("depth_640_480"), EAttribute::Depth_640_480},
		{TEXT("depth_128_128"), EAttribute::Depth_128_128}};

UTextureRenderTarget2D *RenderTarget_RGBA8_3840_2160;
UTextureRenderTarget2D *RenderTarget_RGBA8_1280_1024;
UTextureRenderTarget2D *RenderTarget_RGBA8_640_480;
UTextureRenderTarget2D *RenderTarget_RGBA8_128_128;

UTextureRenderTarget2D *RenderTarget_R16_3840_2160;
UTextureRenderTarget2D *RenderTarget_R16_1280_1024;
UTextureRenderTarget2D *RenderTarget_R16_640_480;
UTextureRenderTarget2D *RenderTarget_R16_128_128;

static void BindMetaData(const TSharedPtr<FJsonObject> &MetaDataJson,
						 const TPair<AActor *, FAttributeContainer> &Object,
						 TMap<FString, AActor *> &CachedActors,
						 TMap<FString, UActorComponent *> &CachedComponents,
						 TMap<FString, TMap<UMultiverseAnim *, FName>> &CachedBoneNames)
{
	TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
	if (Object.Key != nullptr)
	{
		for (const EAttribute &Attribute : Object.Value.Attributes)
		{
			const FString AttributeName = *AttributeStringMap.FindKey(Attribute);
			switch (Attribute)
			{
			case EAttribute::Position:
				AttributeJsonArray.Add(MakeShareable(new FJsonValueString(AttributeName)));
				break;

			case EAttribute::Quaternion:
				AttributeJsonArray.Add(MakeShareable(new FJsonValueString(AttributeName)));
				break;

			case EAttribute::RGB_3840_2160:
			case EAttribute::RGB_1280_1024:
			case EAttribute::RGB_640_480:
			case EAttribute::RGB_128_128:
			case EAttribute::Depth_3840_2160:
			case EAttribute::Depth_1280_1024:
			case EAttribute::Depth_640_480:
			case EAttribute::Depth_128_128:
			{
				TArray<USceneCaptureComponent2D *> SceneCaptureComponents;
				Object.Key->GetComponents(SceneCaptureComponents);
				for (USceneCaptureComponent2D *SceneCaptureComponent : SceneCaptureComponents)
				{
					if (SceneCaptureComponent->TextureTarget != nullptr)
					{
						continue;
					}
					AttributeJsonArray.Add(MakeShareable(new FJsonValueString(AttributeName)));
					SceneCaptureComponent->ComponentTags.Add(*AttributeName);
					if (Attribute == EAttribute::RGB_3840_2160 || Attribute == EAttribute::Depth_3840_2160)
					{
						if (Attribute == EAttribute::RGB_3840_2160)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_RGBA8_3840_2160, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
						}
						else if (Attribute == EAttribute::Depth_3840_2160)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_R16_3840_2160, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
						}
					}
					else if (Attribute == EAttribute::RGB_1280_1024 || Attribute == EAttribute::Depth_1280_1024)
					{
						if (Attribute == EAttribute::RGB_1280_1024)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_RGBA8_1280_1024, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
						}
						else if (Attribute == EAttribute::Depth_1280_1024)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_R16_1280_1024, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
						}
					}
					else if (Attribute == EAttribute::RGB_640_480 || Attribute == EAttribute::Depth_640_480)
					{
						if (Attribute == EAttribute::RGB_640_480)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_RGBA8_640_480, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
						}
						else if (Attribute == EAttribute::Depth_640_480)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_R16_640_480, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
						}
					}
					else if (Attribute == EAttribute::RGB_128_128 || Attribute == EAttribute::Depth_128_128)
					{
						if (Attribute == EAttribute::RGB_128_128)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_RGBA8_128_128, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
						}
						else if (Attribute == EAttribute::Depth_128_128)
						{
							SceneCaptureComponent->TextureTarget = DuplicateObject(RenderTarget_R16_128_128, Object.Key, *AttributeName);
							SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
							SceneCaptureComponent->TextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
						}
					}
					break;
				}
				break;
			}

			default:
				break;
			}
		}
	}
	if (Object.Key->IsA(ASkeletalMeshActor::StaticClass()))
	{
		const FString ObjectName = Object.Value.ObjectPrefix + Object.Value.ObjectName + Object.Value.ObjectSuffix;
		CachedActors.Add(ObjectName, Object.Key);
		MetaDataJson->SetArrayField(ObjectName, AttributeJsonArray);

		ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(Object.Key);
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
					FString BoneNameStr = Object.Value.ObjectPrefix + BoneName.ToString() + Object.Value.ObjectSuffix;
					if (((BoneNameStr.EndsWith(TEXT("_revolute_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_revolute_bone"))) ||
						 (BoneNameStr.EndsWith(TEXT("_continuous_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_continuous_bone")))) &&
						Object.Value.Attributes.Contains(EAttribute::JointRvalue))
					{
						AttributeJsonArray = {MakeShareable(new FJsonValueString(TEXT("joint_rvalue")))};
						if (!CachedBoneNames.Contains(BoneNameStr))
						{
							CachedBoneNames.Add(BoneNameStr, TMap<UMultiverseAnim *, FName>());
						}
						CachedBoneNames[BoneNameStr].Add(MultiverseAnim, BoneName);
						MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
					}
					else if ((BoneNameStr.EndsWith(TEXT("_prismatic_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_prismatic_bone"))) &&
							 Object.Value.Attributes.Contains(EAttribute::JointTvalue))
					{
						AttributeJsonArray = {MakeShareable(new FJsonValueString(TEXT("joint_tvalue")))};
						if (!CachedBoneNames.Contains(BoneNameStr))
						{
							CachedBoneNames.Add(BoneNameStr, TMap<UMultiverseAnim *, FName>());
						}
						CachedBoneNames[BoneNameStr].Add(MultiverseAnim, BoneName);
						MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
					}
				}
			}
			else
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("SkeletalMeshActor %s does not contain a MultiverseAnim."), *Object.Value.ObjectName)
			}
		}
		else
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("SkeletalMeshActor %s does not contain a UStaticMeshComponent."), *Object.Value.ObjectName)
		}
	}
	else if (Object.Key->IsA(APawn::StaticClass()))
	{
#ifdef WIN32
		for (const FString &Tag : {TEXT("LeftHand"), TEXT("RightHand")})
		{
			TArray<UActorComponent *> ActorComponents = Object.Key->GetComponentsByTag(UOculusXRHandComponent::StaticClass(), *Tag);
			if (ActorComponents.Num() == 0)
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("%s not found"), *Tag)
				continue;
			}
			else if (ActorComponents.Num() > 1)
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("Found %d %s, ignore all"), ActorComponents.Num(), *Tag)
				continue;
			}

			for (TPair<EOculusXRBone, FName> &BoneNameMapping : Cast<UOculusXRHandComponent>(ActorComponents[0])->BoneNameMappings)
			{
				const FString BoneNameStr = Tag + TEXT("_") + UEnum::GetDisplayValueAsText(BoneNameMapping.Key).ToString().Replace(TEXT(" "), TEXT(""));
				BoneNameMapping.Value = *BoneNameStr;
				TArray<TSharedPtr<FJsonValue>> FingerAttributeJsonArray;
				for (const EAttribute &Attribute : Object.Value.Attributes)
				{
					const FString AttributeName = *AttributeStringMap.FindKey(Attribute);
					switch (Attribute)
					{
					case EAttribute::Position:
						FingerAttributeJsonArray.Add(MakeShareable(new FJsonValueString(AttributeName)));
						break;

					case EAttribute::Quaternion:
						FingerAttributeJsonArray.Add(MakeShareable(new FJsonValueString(AttributeName)));
						break;

					default:
						break;
					}
				}
#if WITH_EDITOR
				MetaDataJson->SetArrayField(BoneNameStr, FingerAttributeJsonArray);
#else
				const FString BoneNameStrModified = Tag + TEXT("_") + UEnum::GetDisplayValueAsText(BoneNameMapping.Key).ToString().Replace(TEXT(" "), TEXT("")).Replace(TEXT("_"), TEXT(""));
				MetaDataJson->SetArrayField(BoneNameStrModified, FingerAttributeJsonArray);
#endif
			}
		}
		const FString ObjectName = Object.Value.ObjectPrefix + Object.Value.ObjectName + Object.Value.ObjectSuffix;
		CachedActors.Add(ObjectName, Object.Key);
		MetaDataJson->SetArrayField(ObjectName, AttributeJsonArray);
#endif
	}
	else if (Object.Key != nullptr)
	{
		const FString ObjectName = Object.Value.ObjectPrefix + Object.Value.ObjectName + Object.Value.ObjectSuffix;
		CachedActors.Add(ObjectName, Object.Key);
		MetaDataJson->SetArrayField(ObjectName, AttributeJsonArray);
	}
}

static void BindMetaData(const TSharedPtr<FJsonObject> &MetaDataJson,
						 TPair<FString, FAttributeDataContainer> &CustomObject)
{
	TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
	for (TPair<EAttribute, FDataContainer> &Attribute : CustomObject.Value.Attributes)
	{
		Attribute.Value.Data = AttributeDoubleDataMap[Attribute.Key];
		const FString AttributeName = *AttributeStringMap.FindKey(Attribute.Key);
		AttributeJsonArray.Add(MakeShareable(new FJsonValueString(AttributeName)));
	}
	MetaDataJson->SetArrayField(CustomObject.Key, AttributeJsonArray);
}

static void BindDataArray(TArray<TPair<FString, EAttribute>> &DataArray,
						  const TPair<AActor *, FAttributeContainer> &Object)
{
	if (ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(Object.Key))
	{
		const FString ObjectName = Object.Value.ObjectPrefix + Object.Value.ObjectName + Object.Value.ObjectSuffix;
		for (const EAttribute &Attribute : Object.Value.Attributes)
		{
			if (Attribute == EAttribute::Position ||
				Attribute == EAttribute::Quaternion ||
				Attribute == EAttribute::RGB_3840_2160 ||
				Attribute == EAttribute::RGB_1280_1024 ||
				Attribute == EAttribute::RGB_640_480 ||
				Attribute == EAttribute::RGB_128_128 ||
				Attribute == EAttribute::Depth_3840_2160 ||
				Attribute == EAttribute::Depth_1280_1024 ||
				Attribute == EAttribute::Depth_640_480 ||
				Attribute == EAttribute::Depth_128_128)
			{
				const TPair<FString, EAttribute> NewData(ObjectName, Attribute);
				if (!DataArray.Contains(NewData))
				{
					DataArray.Add(NewData);
				}
			}
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
						const TPair<FString, EAttribute> NewData(Object.Value.ObjectPrefix + BoneNameStr + Object.Value.ObjectSuffix, EAttribute::JointRvalue);
						if (!DataArray.Contains(NewData))
						{
							DataArray.Add(NewData);
						}
					}
					else if ((BoneNameStr.EndsWith(TEXT("_prismatic_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_prismatic_bone"))) &&
							 Object.Value.Attributes.Contains(EAttribute::JointTvalue))
					{
						const TPair<FString, EAttribute> NewData(Object.Value.ObjectPrefix + BoneNameStr + Object.Value.ObjectSuffix, EAttribute::JointTvalue);
						if (!DataArray.Contains(NewData))
						{
							DataArray.Add(NewData);
						}
					}
				}
			}
			else
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("SkeletalMeshActor %s does not contain a MultiverseAnim."), *Object.Value.ObjectName)
			}
		}
		else
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("SkeletalMeshActor %s does not contain a UStaticMeshComponent."), *Object.Value.ObjectName)
		}

		DataArray.Sort([](const TPair<FString, EAttribute> &DataA, const TPair<FString, EAttribute> &DataB)
					   { return DataB.Key.Compare(DataA.Key) > 0 || (DataB.Key.Compare(DataA.Key) == 0 && DataB.Value > DataA.Value); });
	}
	else if (Object.Key->IsA(APawn::StaticClass()))
	{
		const FString ObjectName = Object.Value.ObjectPrefix + Object.Value.ObjectName + Object.Value.ObjectSuffix;
		TArray<UCameraComponent *> CameraComponents;
		Object.Key->GetComponents(CameraComponents);
		if (CameraComponents.Num() == 1)
		{
			if (Object.Value.Attributes.Contains(EAttribute::Position))
			{
				const TPair<FString, EAttribute> NewData(ObjectName, EAttribute::Position);
				if (!DataArray.Contains(NewData))
				{
					DataArray.Add(NewData);
				}
			}
			if (Object.Value.Attributes.Contains(EAttribute::Quaternion))
			{
				const TPair<FString, EAttribute> NewData(ObjectName, EAttribute::Quaternion);
				if (!DataArray.Contains(NewData))
				{
					DataArray.Add(NewData);
				}
			}
		}
		else
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("CameraComponents.Num() of %s = %d"), *Object.Value.ObjectName, CameraComponents.Num())
		}

		TArray<USkeletalMeshComponent *> SkeletalMeshComponents;
		Object.Key->GetComponents(SkeletalMeshComponents, true);
		for (USkeletalMeshComponent *SkeletalMeshComponent : SkeletalMeshComponents)
		{
			if (Object.Value.Attributes.Contains(EAttribute::Position))
			{
				const TPair<FString, EAttribute> NewData(Object.Value.ObjectPrefix + SkeletalMeshComponent->GetName() + Object.Value.ObjectSuffix, EAttribute::Position);
				if (!DataArray.Contains(NewData))
				{
					DataArray.Add(NewData);
				}
			}
			if (Object.Value.Attributes.Contains(EAttribute::Quaternion))
			{
				const TPair<FString, EAttribute> NewData(Object.Value.ObjectPrefix + SkeletalMeshComponent->GetName() + Object.Value.ObjectSuffix, EAttribute::Quaternion);
				if (!DataArray.Contains(NewData))
				{
					DataArray.Add(NewData);
				}
			}
		}

		if (SkeletalMeshComponents.Num() == 0)
		{
			for (const EAttribute &Attribute : Object.Value.Attributes)
			{
				if (Attribute == EAttribute::Position || Attribute == EAttribute::Quaternion)
				{
					TPair<FString, EAttribute> NewData(Object.Value.ObjectName, Attribute);
					if (!DataArray.Contains(NewData))
					{
						DataArray.Add(NewData);
					}
				}
			}
		}

#ifdef WIN32
		for (const FString &Tag : {TEXT("LeftHand"), TEXT("RightHand")})
		{
			TArray<UActorComponent *> ActorComponents = Object.Key->GetComponentsByTag(UOculusXRHandComponent::StaticClass(), *Tag);
			if (ActorComponents.Num() == 0)
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("%s not found"), *Tag)
				continue;
			}
			else if (ActorComponents.Num() > 1)
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("Found %d %s, ignore all"), ActorComponents.Num(), *Tag)
				continue;
			}

			for (const TPair<EOculusXRBone, FName> &BoneNameMapping : Cast<UOculusXRHandComponent>(ActorComponents[0])->BoneNameMappings)
			{
				if (Object.Value.Attributes.Contains(EAttribute::Position))
				{
					const TPair<FString, EAttribute> NewData(BoneNameMapping.Value.ToString(), EAttribute::Position);
					if (!DataArray.Contains(NewData))
					{
						DataArray.Add(NewData);
					}
				}
				if (Object.Value.Attributes.Contains(EAttribute::Quaternion))
				{
					const TPair<FString, EAttribute> NewData(BoneNameMapping.Value.ToString(), EAttribute::Quaternion);
					if (!DataArray.Contains(NewData))
					{
						DataArray.Add(NewData);
					}
				}
			}
		}
#endif

		DataArray.Sort([](const TPair<FString, EAttribute> &DataA, const TPair<FString, EAttribute> &DataB)
					   { return DataB.Key.Compare(DataA.Key) > 0 || (DataB.Key.Compare(DataA.Key) == 0 && DataB.Value > DataA.Value); });
	}
	else if (Object.Key != nullptr)
	{
		const FString ObjectName = Object.Value.ObjectPrefix + Object.Value.ObjectName + Object.Value.ObjectSuffix;
		for (const EAttribute &Attribute : Object.Value.Attributes)
		{
			const TPair<FString, EAttribute> NewData(ObjectName, Attribute);
			if (!DataArray.Contains(NewData))
			{
				DataArray.Add(NewData);
			}
		}

		DataArray.Sort([](const TPair<FString, EAttribute> &DataA, const TPair<FString, EAttribute> &DataB)
					   { return DataB.Key.Compare(DataA.Key) > 0 || (DataB.Key.Compare(DataA.Key) == 0 && DataB.Value > DataA.Value); });
	}
}

static void BindDataArray(TArray<TPair<FString, EAttribute>> &DataArray,
						  const TPair<FString, FAttributeDataContainer> &CustomObject)
{
	for (const TPair<EAttribute, FDataContainer> &Attribute : CustomObject.Value.Attributes)
	{
		const TPair<FString, EAttribute> NewData(CustomObject.Key, Attribute.Key);
		if (!DataArray.Contains(NewData))
		{
			DataArray.Add(NewData);
		}
	}
	DataArray.Sort([](const TPair<FString, EAttribute> &DataA, const TPair<FString, EAttribute> &DataB)
				   { return DataB.Key.Compare(DataA.Key) > 0 || (DataB.Key.Compare(DataA.Key) == 0 && DataB.Value > DataA.Value); });
}

FMultiverseClient::FMultiverseClient()
{
	AttributeUint8DataMap[EAttribute::RGB_3840_2160].Init(0, 3840 * 2160 * 3);
	AttributeUint8DataMap[EAttribute::RGB_1280_1024].Init(0, 1280 * 1024 * 3);
	AttributeUint8DataMap[EAttribute::RGB_640_480].Init(0, 640 * 480 * 3);
	AttributeUint8DataMap[EAttribute::RGB_128_128].Init(0, 128 * 128 * 3);

	AttributeUint16DataMap[EAttribute::Depth_3840_2160].Init(0, 3840 * 2160);
	AttributeUint16DataMap[EAttribute::Depth_1280_1024].Init(0, 1280 * 1024);
	AttributeUint16DataMap[EAttribute::Depth_640_480].Init(0, 640 * 480);
	AttributeUint16DataMap[EAttribute::Depth_128_128].Init(0, 128 * 128);

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

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_RGBA8_3840_2160(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_RGBA8_3840_2160.RT_RGBA8_3840_2160'"));
	if (RenderTargetAsset_RGBA8_3840_2160.Succeeded())
	{
		RenderTarget_RGBA8_3840_2160 = RenderTargetAsset_RGBA8_3840_2160.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_RGBA8_3840_2160"))
	}

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_RGBA8_1280_1024(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_RGBA8_1280_1024.RT_RGBA8_1280_1024'"));
	if (RenderTargetAsset_RGBA8_1280_1024.Succeeded())
	{
		RenderTarget_RGBA8_1280_1024 = RenderTargetAsset_RGBA8_1280_1024.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_RGBA8_1280_1024"))
	}

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_RGBA8_640_480(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_RGBA8_640_480.RT_RGBA8_640_480'"));
	if (RenderTargetAsset_RGBA8_640_480.Succeeded())
	{
		RenderTarget_RGBA8_640_480 = RenderTargetAsset_RGBA8_640_480.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_RGBA8_640_480"))
	}

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_RGBA8_128_128(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_RGBA8_128_128.RT_RGBA8_128_128'"));
	if (RenderTargetAsset_RGBA8_128_128.Succeeded())
	{
		RenderTarget_RGBA8_128_128 = RenderTargetAsset_RGBA8_128_128.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_RGBA8_128_128"))
	}

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_R16_3840_2160(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_RGBA8_3840_2160.RT_RGBA8_3840_2160'"));
	if (RenderTargetAsset_R16_3840_2160.Succeeded())
	{
		RenderTarget_RGBA8_3840_2160 = RenderTargetAsset_R16_3840_2160.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_RGBA8_3840_2160"))
	}

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_R16_1280_1024(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_R16_1280_1024.RT_R16_1280_1024'"));
	if (RenderTargetAsset_R16_1280_1024.Succeeded())
	{
		RenderTarget_R16_1280_1024 = RenderTargetAsset_R16_1280_1024.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_R16_1280_1024"))
	}

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_R16_640_480(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_R16_640_480.RT_R16_640_480'"));
	if (RenderTargetAsset_R16_640_480.Succeeded())
	{
		RenderTarget_R16_640_480 = RenderTargetAsset_R16_640_480.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_R16_640_480"))
	}

	ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> RenderTargetAsset_R16_128_128(TEXT("/Script/Engine.TextureRenderTarget2D'/MultiverseConnector/Rendering/RT_R16_128_128.RT_R16_128_128'"));
	if (RenderTargetAsset_R16_128_128.Succeeded())
	{
		RenderTarget_R16_128_128 = RenderTargetAsset_R16_128_128.Object;
	}
	else
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Failed to find RT_R16_128_128"))
	}
}

void FMultiverseClient::Init(const FString &ServerHost, const FString &ServerPort, const FString &ClientPort,
							 const FString &InWorldName, const FString &InSimulationName,
							 TMap<AActor *, FAttributeContainer> &InSendObjects,
							 TMap<AActor *, FAttributeContainer> &InReceiveObjects,
							 TMap<FString, FAttributeDataContainer> *InSendCustomObjectsPtr,
							 TMap<FString, FAttributeDataContainer> *InReceiveCustomObjectsPtr,
							 UWorld *InWorld)
{
	SendObjects = InSendObjects;
	ReceiveObjects = InReceiveObjects;
	SendCustomObjectsPtr = InSendCustomObjectsPtr;
	ReceiveCustomObjectsPtr = InReceiveCustomObjectsPtr;
	World = InWorld;
	WorldName = InWorldName;
	SimulationName = InSimulationName;

	host = TCHAR_TO_UTF8(*ServerHost);
	server_port = TCHAR_TO_UTF8(*ServerPort);
	client_port = TCHAR_TO_UTF8(*ClientPort);

	connect();

	StartTime = FPlatformTime::Seconds();
}

TMap<FString, FApiCallbacks> FMultiverseClient::CallApis(const TMap<FString, FApiCallbacks> &SimulationApiCallbacks)
{
	RequestMetaDataJson = MakeShareable(new FJsonObject);
	const TSharedPtr<FJsonObject> SimulationApiCallbacksJson = MakeShareable(new FJsonObject);
	for (const TPair<FString, FApiCallbacks> &SimulationApiCallback : SimulationApiCallbacks)
	{
		TArray<TSharedPtr<FJsonValue>> SimulationApiCallbacksJsonValues;
		for (const FApiCallback &ApiCallback : SimulationApiCallback.Value.ApiCallbacks)
		{
			TArray<TSharedPtr<FJsonValue>> ApiCallbackArgumentsJsonValues;
			for (const FString &Argument : ApiCallback.Arguments)
			{
				ApiCallbackArgumentsJsonValues.Add(MakeShareable(new FJsonValueString(Argument)));
			}
			TSharedPtr<FJsonObject> SimulationApiCallbackJson = MakeShareable(new FJsonObject);
			SimulationApiCallbackJson->SetArrayField(ApiCallback.Name, ApiCallbackArgumentsJsonValues);
			SimulationApiCallbacksJsonValues.Add(MakeShareable(new FJsonValueObject(SimulationApiCallbackJson)));
		}
		SimulationApiCallbacksJson->SetArrayField(SimulationApiCallback.Key, SimulationApiCallbacksJsonValues);
	}
	RequestMetaDataJson->SetObjectField(TEXT("api_callbacks"), SimulationApiCallbacksJson);

	if (ResponseMetaDataJson.IsValid())
	{
		ResponseMetaDataJson->RemoveField(TEXT("api_callbacks_response"));
	}

	bComputingRequestAndResponseMetaData = true;
	communicate(true);
	while (bComputingRequestAndResponseMetaData)
	{
		UE_LOG(LogMultiverseClient, Warning, TEXT("Waiting for api_callbacks_response..."))
	}

	while (!ResponseMetaDataJson.IsValid() || ResponseMetaDataJson->Values.IsEmpty() || !ResponseMetaDataJson->HasField(TEXT("api_callbacks_response")))
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("Waiting for api_callbacks_response..."))
	}

	TMap<FString, FApiCallbacks> ApiCallbacksResponse;
	TSharedPtr<FJsonObject> SimulationApiCallbacksResponseJson = ResponseMetaDataJson->GetObjectField(TEXT("api_callbacks_response"));
	for (const TPair<FString, FApiCallbacks> &SimulationApiCallback : SimulationApiCallbacks)
	{
		if (!SimulationApiCallbacksResponseJson->HasField(SimulationApiCallback.Key))
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("api_callbacks_response does not contain simulation [%s]"), *SimulationApiCallback.Key)
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>> ApiCallbacksResponseJsonValue = SimulationApiCallbacksResponseJson->GetArrayField(SimulationApiCallback.Key);
		FApiCallbacks ApiCallbacks;
		for (const TSharedPtr<FJsonValue> &ApiCallbackResponseJsonValue : ApiCallbacksResponseJsonValue)
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>> &ApiCallbackResponse : ApiCallbackResponseJsonValue->AsObject()->Values)
			{
				TArray<FString> Arguments;
				for (const TSharedPtr<FJsonValue> &Argument : ApiCallbackResponse.Value->AsArray())
				{
					Arguments.Add(Argument->AsString());
				}
				ApiCallbacks.ApiCallbacks.Add(FApiCallback{ApiCallbackResponse.Key, Arguments});
			}
		}
		ApiCallbacksResponse.Add(SimulationApiCallback.Key, ApiCallbacks);
	}

	return ApiCallbacksResponse;
}

UMaterial *FMultiverseClient::GetMaterial(const FLinearColor &Color) const
{
	const FString ColorName = TEXT("M_") + ColorMap[Color];
	return Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *(TEXT("Material'/MultiverseConnector/Assets/Materials/") + ColorName + TEXT(".") + ColorName + TEXT("'"))));
}

bool FMultiverseClient::compute_request_and_response_meta_data()
{
	bComputingRequestAndResponseMetaData = true;
	ResponseMetaDataJson = MakeShareable(new FJsonObject);
	if (response_meta_data_str.empty())
	{
		bComputingRequestAndResponseMetaData = false;
		return false;
	}

	FString ResponseMetaDataString(response_meta_data_str.c_str());
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseMetaDataString);

	// UE_LOG(LogMultiverseClient, Log, TEXT("%s"), *ResponseMetaDataString)

	bool bParseSuccess = FJsonSerializer::Deserialize(Reader, ResponseMetaDataJson) &&
						 ResponseMetaDataJson->HasField(TEXT("time")) &&
						 ResponseMetaDataJson->GetNumberField(TEXT("time")) >= 0.0;

	bComputingRequestAndResponseMetaData = false;

	return bParseSuccess;
}

void FMultiverseClient::compute_request_buffer_sizes(std::map<std::string, size_t> &send_buffer_size, std::map<std::string, size_t> &receive_buffer_size) const
{
	TMap<FString, TMap<FString, size_t>> RequestBufferSizes = {{TEXT("send"), {{TEXT("double"), 0}, {TEXT("uint8"), 0}, {TEXT("uint16"), 0}}}, {TEXT("receive"), {{TEXT("double"), 0}, {TEXT("uint8"), 0}, {TEXT("uint16"), 0}}}};

	for (TPair<FString, TMap<FString, size_t>> &RequestBufferSize : RequestBufferSizes)
	{
		RequestBufferSize.Value[TEXT("double")] = 0;
		RequestBufferSize.Value[TEXT("uint8")] = 0;
		RequestBufferSize.Value[TEXT("uint16")] = 0;
		if (!RequestMetaDataJson->HasField(RequestBufferSize.Key))
		{
			continue;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>> &ObjectJson : RequestMetaDataJson->GetObjectField(RequestBufferSize.Key)->Values)
		{
			if (ObjectJson.Key.Compare(TEXT("")) == 0 ||
				RequestBufferSize.Value[TEXT("double")] == -1 ||
				RequestBufferSize.Value[TEXT("uint8")] == -1 ||
				RequestBufferSize.Value[TEXT("uint16")] == -1)
			{
				RequestBufferSize.Value[TEXT("double")] = -1;
				RequestBufferSize.Value[TEXT("uint8")] = -1;
				RequestBufferSize.Value[TEXT("uint16")] = -1;
				break;
			}

			for (const TSharedPtr<FJsonValue> &ObjectAttributeJson : ObjectJson.Value->AsArray())
			{
				const FString ObjectAttribute = ObjectAttributeJson->AsString();
				if (ObjectAttribute.Compare(TEXT("")) == 0)
				{
					RequestBufferSize.Value[TEXT("double")] = -1;
					RequestBufferSize.Value[TEXT("uint8")] = -1;
					RequestBufferSize.Value[TEXT("uint16")] = -1;
					break;
				}

				if (AttributeStringMap.Contains(ObjectAttribute) && AttributeDoubleDataMap.Contains(AttributeStringMap[ObjectAttribute]))
				{
					RequestBufferSize.Value[TEXT("double")] += AttributeDoubleDataMap[AttributeStringMap[ObjectAttribute]].Num();
				}
				else if (AttributeStringMap.Contains(ObjectAttribute) && AttributeUint8DataMap.Contains(AttributeStringMap[ObjectAttribute]))
				{
					RequestBufferSize.Value[TEXT("uint8")] += AttributeUint8DataMap[AttributeStringMap[ObjectAttribute]].Num();
				}
				else if (AttributeStringMap.Contains(ObjectAttribute) && AttributeUint16DataMap.Contains(AttributeStringMap[ObjectAttribute]))
				{
					RequestBufferSize.Value[TEXT("uint16")] += AttributeUint16DataMap[AttributeStringMap[ObjectAttribute]].Num();
				}
			}
		}
	}

	send_buffer_size = {{"double", RequestBufferSizes[TEXT("send")][TEXT("double")]},
						{"uint8", RequestBufferSizes[TEXT("send")][TEXT("uint8")]},
						{"uint16", RequestBufferSizes[TEXT("send")][TEXT("uint16")]}};
	receive_buffer_size = {{"double", RequestBufferSizes[TEXT("receive")][TEXT("double")]},
						   {"uint8", RequestBufferSizes[TEXT("receive")][TEXT("uint8")]},
						   {"uint16", RequestBufferSizes[TEXT("receive")][TEXT("uint16")]}};
}

void FMultiverseClient::compute_response_buffer_sizes(std::map<std::string, size_t> &send_buffer_size, std::map<std::string, size_t> &receive_buffer_size) const
{
	TMap<FString, TMap<FString, size_t>> ResponseBufferSizes = {{TEXT("send"), {{TEXT("double"), 0}, {TEXT("uint8"), 0}, {TEXT("uint16"), 0}}}, {TEXT("receive"), {{TEXT("double"), 0}, {TEXT("uint8"), 0}, {TEXT("uint16"), 0}}}};

	for (TPair<FString, TMap<FString, size_t>> &ResponseBufferSize : ResponseBufferSizes)
	{
		ResponseBufferSize.Value[TEXT("double")] = 0;
		ResponseBufferSize.Value[TEXT("uint8")] = 0;
		ResponseBufferSize.Value[TEXT("uint16")] = 0;
		if (!ResponseMetaDataJson->HasField(ResponseBufferSize.Key))
		{
			continue;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>> &ObjectJson : ResponseMetaDataJson->GetObjectField(ResponseBufferSize.Key)->Values)
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>> &ObjectData : ObjectJson.Value->AsObject()->Values)
			{
				const FString ObjectAttribute = ObjectData.Key;
				if (AttributeStringMap.Contains(ObjectAttribute) && AttributeDoubleDataMap.Contains(AttributeStringMap[ObjectAttribute]))
				{
					ResponseBufferSize.Value[TEXT("double")] += ObjectData.Value->AsArray().Num();
				}
				else if (AttributeStringMap.Contains(ObjectAttribute) && AttributeUint8DataMap.Contains(AttributeStringMap[ObjectAttribute]))
				{
					ResponseBufferSize.Value[TEXT("uint8")] += ObjectData.Value->AsArray().Num();
				}
				else if (AttributeStringMap.Contains(ObjectAttribute) && AttributeUint16DataMap.Contains(AttributeStringMap[ObjectAttribute]))
				{
					ResponseBufferSize.Value[TEXT("uint16")] += ObjectData.Value->AsArray().Num();
				}
			}
		}
	}

	send_buffer_size = {{"double", ResponseBufferSizes[TEXT("send")][TEXT("double")]},
						{"uint8", ResponseBufferSizes[TEXT("send")][TEXT("uint8")]},
						{"uint16", ResponseBufferSizes[TEXT("send")][TEXT("uint16")]}};
	receive_buffer_size = {{"double", ResponseBufferSizes[TEXT("receive")][TEXT("double")]},
						   {"uint8", ResponseBufferSizes[TEXT("receive")][TEXT("uint8")]},
						   {"uint16", ResponseBufferSizes[TEXT("receive")][TEXT("uint16")]}};
}

bool FMultiverseClient::init_objects(bool from_request_meta_data)
{
	SendObjects.Remove(nullptr);
	ReceiveObjects.Remove(nullptr);

	if (SendObjects.Num() > 0)
	{
		SendObjects.ValueSort([](const FAttributeContainer &AttributeContainerA, const FAttributeContainer &AttributeContainerB)
							  { return AttributeContainerB.ObjectName.Compare(AttributeContainerA.ObjectName) > 0; });
	}
	if (ReceiveObjects.Num() > 0)
	{
		ReceiveObjects.ValueSort([](const FAttributeContainer &AttributeContainerA, const FAttributeContainer &AttributeContainerB)
								 { return AttributeContainerB.ObjectName.Compare(AttributeContainerA.ObjectName) > 0; });
	}

	for (TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
	{
		SendObject.Value.Attributes.Sort([](const EAttribute &AttributeA, const EAttribute &AttributeB)
										 { return AttributeB > AttributeA; });
	}
	for (TPair<AActor *, FAttributeContainer> &ReceiveObject : ReceiveObjects)
	{
		ReceiveObject.Value.Attributes.Sort([](const EAttribute &AttributeA, const EAttribute &AttributeB)
											{ return AttributeB > AttributeA; });
	}

	if (World == nullptr)
	{
		UE_LOG(LogMultiverseClient, Error, TEXT("World is nullptr"))
		return false;
	}

	return SendObjects.Num() > 0 || ReceiveObjects.Num() > 0;
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
	TSharedPtr<FJsonObject> ApiCallbacks;
	TArray<TSharedPtr<FJsonValue>> ApiCallbacksResponse;
	if (RequestMetaDataJson && RequestMetaDataJson->HasField(TEXT("api_callbacks")))
	{
		ApiCallbacks = RequestMetaDataJson->GetObjectField(TEXT("api_callbacks"));
	}
	if (RequestMetaDataJson && RequestMetaDataJson->HasField(TEXT("api_callbacks_response")))
	{
		ApiCallbacksResponse = RequestMetaDataJson->GetArrayField(TEXT("api_callbacks_response"));
	}

	RequestMetaDataJson = MakeShareable(new FJsonObject);

	if (ApiCallbacks)
	{
		RequestMetaDataJson->SetObjectField(TEXT("api_callbacks"), ApiCallbacks);
	}
	if (ApiCallbacksResponse.Num() > 0)
	{
		RequestMetaDataJson->SetArrayField(TEXT("api_callbacks_response"), ApiCallbacksResponse);
	}

	TSharedPtr<FJsonObject> MetaDataJson = MakeShareable(new FJsonObject);
	MetaDataJson->SetStringField(TEXT("world_name"), WorldName);
	MetaDataJson->SetStringField(TEXT("simulation_name"), SimulationName);
	MetaDataJson->SetStringField(TEXT("time_unit"), TEXT("s"));
	MetaDataJson->SetStringField(TEXT("length_unit"), TEXT("cm"));
	MetaDataJson->SetStringField(TEXT("angle_unit"), TEXT("deg"));
	MetaDataJson->SetStringField(TEXT("handedness"), TEXT("lhs"));
	MetaDataJson->SetStringField(TEXT("force_unit"), TEXT("N"));

	RequestMetaDataJson->SetObjectField(TEXT("meta_data"), MetaDataJson);

	RequestMetaDataJson->SetObjectField(TEXT("send"), MakeShareable(new FJsonObject));
	RequestMetaDataJson->SetObjectField(TEXT("receive"), MakeShareable(new FJsonObject));

	for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
	{
		if (SendObject.Key == nullptr)
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("Ignore None Object in SendObjects"))
			continue;
		}

		BindMetaData(RequestMetaDataJson->GetObjectField(TEXT("send")), SendObject, CachedActors, CachedComponents, CachedBoneNames);
	}

	for (const TPair<AActor *, FAttributeContainer> &ReceiveObject : ReceiveObjects)
	{
		if (ReceiveObject.Key == nullptr)
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("Ignore None Object in ReceiveObjects"))
			continue;
		}

		BindMetaData(RequestMetaDataJson->GetObjectField(TEXT("receive")), ReceiveObject, CachedActors, CachedComponents, CachedBoneNames);
	}

	for (TPair<FString, FAttributeDataContainer> &SendCustomObject : *SendCustomObjectsPtr)
	{
		BindMetaData(RequestMetaDataJson->GetObjectField(TEXT("send")), SendCustomObject);
	}

	for (TPair<FString, FAttributeDataContainer> &ReceiveCustomObject : *ReceiveCustomObjectsPtr)
	{
		BindMetaData(RequestMetaDataJson->GetObjectField(TEXT("receive")), ReceiveCustomObject);
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

	UE_LOG(LogMultiverseClient, Log, TEXT("%s"), *RequestMetaDataString)
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
		const FString AttributeName = *AttributeStringMap.FindKey(SendData.Value);
		if ((*SendCustomObjectsPtr).Contains(SendData.Key))
		{
			TArray<TSharedPtr<FJsonValue>> CustomData = ResponseSendObjects->GetArrayField(AttributeName);
			for (TPair<EAttribute, FDataContainer> &Attribute : (*SendCustomObjectsPtr)[SendData.Key].Attributes)
			{
				if (Attribute.Value.Data.Num() != CustomData.Num())
				{
					UE_LOG(LogMultiverseClient, Warning, TEXT("Data Size from Client [%d] mismatch with Data Size from Server [%d]"), Attribute.Value.Data.Num(), CustomData.Num())
					continue;
				}
				for (size_t i = 0; i < Attribute.Value.Data.Num(); i++)
				{
					Attribute.Value.Data[i] = CustomData[i]->AsNumber();
				}
			}
		}
		else if (CachedActors.Contains(SendData.Key))
		{
			if (CachedActors[SendData.Key] == nullptr)
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("Ignore None Object in CachedActors"))
				continue;
			}

			switch (SendData.Value)
			{
			case EAttribute::Position:
			{
				if (!ResponseSendObjects->HasField(AttributeName))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> ObjectPosition = ResponseSendObjects->GetArrayField(AttributeName);
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
				if (!ResponseSendObjects->HasField(AttributeName))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> ObjectQuaternion = ResponseSendObjects->GetArrayField(AttributeName);
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
				if (!ResponseSendObjects->HasField(AttributeName))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> JointRvalue = ResponseSendObjects->GetArrayField(AttributeName);
				if (JointRvalue.Num() != 1)
				{
					continue;
				}
				for (TPair<UMultiverseAnim *, FName> &BoneNameMapping : CachedBoneNames[SendData.Key])
				{
					BoneNameMapping.Key->JointPoses[BoneNameMapping.Value].SetRotation(FQuat(FRotator(JointRvalue[0]->AsNumber(), 0.f, 0.f)));
				}
				break;
			}

			case EAttribute::JointTvalue:
			{
				if (!ResponseSendObjects->HasField(AttributeName))
				{
					continue;
				}

				TArray<TSharedPtr<FJsonValue>> JointTvalue = ResponseSendObjects->GetArrayField(AttributeName);
				if (JointTvalue.Num() != 1)
				{
					continue;
				}
				for (TPair<UMultiverseAnim *, FName> &BoneNameMapping : CachedBoneNames[SendData.Key])
				{
					BoneNameMapping.Key->JointPoses[BoneNameMapping.Value].SetTranslation(FVector(0.f, JointTvalue[0]->AsNumber(), 0.f));;
				}
				break;
			}

			default:
				break;
			}
		}
	}
}

void FMultiverseClient::bind_api_callbacks()
{
	if (!ResponseMetaDataJson->HasField(TEXT("api_callbacks")))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue> &ApiCallbackJsonValue : ResponseMetaDataJson->GetArrayField(TEXT("api_callbacks")))
	{
		const TSharedPtr<FJsonObject> ApiCallbackJson = ApiCallbackJsonValue->AsObject();
		for (const TPair<FString, TSharedPtr<FJsonValue>> &ApiCallbackPair : ApiCallbackJson->Values)
		{
			if (ApiCallbackPair.Key.Compare(TEXT("is_unreal")) == 0)
			{
				// UE_LOG(LogMultiverseClient, Log, TEXT("Received!"))
			}
		}
	}
}

void FMultiverseClient::bind_api_callbacks_response()
{
	if (!ResponseMetaDataJson->HasField(TEXT("api_callbacks")))
	{
		return;
	}
	TArray<TSharedPtr<FJsonValue>> ApiCallbacksResponse;
	for (const TSharedPtr<FJsonValue> &ApiCallbackJsonValue : ResponseMetaDataJson->GetArrayField(TEXT("api_callbacks")))
	{
		const TSharedPtr<FJsonObject> ApiCallbackResponseJson = MakeShareable(new FJsonObject);
		for (const TPair<FString, TSharedPtr<FJsonValue>> &ApiCallbackPair : ApiCallbackJsonValue->AsObject()->Values)
		{
			if (ApiCallbackPair.Key.Compare(TEXT("is_unreal")) == 0)
			{
				ApiCallbackResponseJson->SetArrayField(ApiCallbackPair.Key, {MakeShareable(new FJsonValueString(TEXT("true")))});
			}
			else
			{
				ApiCallbackResponseJson->SetArrayField(ApiCallbackPair.Key, {MakeShareable(new FJsonValueString(TEXT("not implemented")))});
			}
			ApiCallbacksResponse.Add(MakeShareable(new FJsonValueObject(ApiCallbackResponseJson)));
		}
	}
	RequestMetaDataJson->SetArrayField(TEXT("api_callbacks_response"), ApiCallbacksResponse);
}

void FMultiverseClient::init_send_and_receive_data()
{
	for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
	{
		if (SendObject.Key == nullptr)
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("Ignore None Object in SendObjects"))
			continue;
		}

		BindDataArray(SendDataArray, SendObject);
	}

	for (const TPair<AActor *, FAttributeContainer> &ReceiveObject : ReceiveObjects)
	{
		if (ReceiveObject.Key == nullptr)
		{
			UE_LOG(LogMultiverseClient, Warning, TEXT("Ignore None Object in ReceiveObjects"))
			continue;
		}

		BindDataArray(ReceiveDataArray, ReceiveObject);
	}

	for (const TPair<FString, FAttributeDataContainer> &SendCustomObject : *SendCustomObjectsPtr)
	{
		BindDataArray(SendDataArray, SendCustomObject);
	}

	for (const TPair<FString, FAttributeDataContainer> &ReceiveCustomObject : *ReceiveCustomObjectsPtr)
	{
		BindDataArray(ReceiveDataArray, ReceiveCustomObject);
	}
}

void FMultiverseClient::bind_send_data()
{
	*world_time = FPlatformTime::Seconds() - StartTime;
	if (*world_time < 0.0)
	{
		*world_time = 0.0;
	}
	double *send_buffer_double_addr = send_buffer.buffer_double.data;
	uint8_t *send_buffer_uint8_addr = send_buffer.buffer_uint8_t.data;
	uint16_t *send_buffer_uint16_addr = send_buffer.buffer_uint16_t.data;

	for (const TPair<FString, EAttribute> &SendData : SendDataArray)
	{
		if ((*SendCustomObjectsPtr).Contains(SendData.Key))
		{
			for (const TPair<EAttribute, FDataContainer> &Attribute : (*SendCustomObjectsPtr)[SendData.Key].Attributes)
			{
				for (const double &Data : Attribute.Value.Data)
				{
					*send_buffer_double_addr++ = Data;
				}
			}
		}
		else if (CachedActors.Contains(SendData.Key))
		{
			if (CachedActors[SendData.Key] == nullptr)
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("Ignore None Object in CachedActors"))
				continue;
			}

			if (SendData.Key.Compare(TEXT("PlayerPawn")) == 0)
			{
				APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
				const FString Tag = TEXT("Head");
				TArray<UActorComponent *> ActorComponents = PlayerPawn->GetComponentsByTag(UCameraComponent::StaticClass(), *Tag);
				if (ActorComponents.Num() != 1)
				{
					UE_LOG(LogMultiverseClient, Warning, TEXT("Found %d %s"), ActorComponents.Num(), *Tag)
					continue;
				}
				UCameraComponent *CameraComponent = Cast<UCameraComponent>(ActorComponents[0]);
				switch (SendData.Value)
				{
				case EAttribute::Position:
				{
					const FVector CameraLocation = CameraComponent->GetComponentLocation();
					*send_buffer_double_addr++ = CameraLocation.X;
					*send_buffer_double_addr++ = CameraLocation.Y;
					*send_buffer_double_addr++ = CameraLocation.Z;
					break;
				}

				case EAttribute::Quaternion:
				{
					const FQuat CameraQuat = CameraComponent->GetComponentQuat();
					*send_buffer_double_addr++ = CameraQuat.W;
					*send_buffer_double_addr++ = CameraQuat.X;
					*send_buffer_double_addr++ = CameraQuat.Y;
					*send_buffer_double_addr++ = CameraQuat.Z;
					break;
				}
				}
			}
			else
			{
				switch (SendData.Value)
				{
				case EAttribute::Position:
				{
					const FVector ActorLocation = CachedActors[SendData.Key]->GetActorLocation();
					*send_buffer_double_addr++ = ActorLocation.X;
					*send_buffer_double_addr++ = ActorLocation.Y;
					*send_buffer_double_addr++ = ActorLocation.Z;
					break;
				}

				case EAttribute::Quaternion:
				{
					const FQuat ActorQuat = CachedActors[SendData.Key]->GetActorQuat();
					*send_buffer_double_addr++ = ActorQuat.W;
					*send_buffer_double_addr++ = ActorQuat.X;
					*send_buffer_double_addr++ = ActorQuat.Y;
					*send_buffer_double_addr++ = ActorQuat.Z;
					break;
				}

				case EAttribute::RGB_3840_2160:
				case EAttribute::RGB_1280_1024:
				case EAttribute::RGB_640_480:
				case EAttribute::RGB_128_128:
				case EAttribute::Depth_3840_2160:
				case EAttribute::Depth_1280_1024:
				case EAttribute::Depth_640_480:
				case EAttribute::Depth_128_128:
				{
					TArray<USceneCaptureComponent2D *> SceneCaptureComponents;
					CachedActors[SendData.Key]->GetComponents(SceneCaptureComponents);
					const FString AttributeName = *AttributeStringMap.FindKey(SendData.Value);
					for (USceneCaptureComponent2D *SceneCaptureComponent : SceneCaptureComponents)
					{
						if (!SceneCaptureComponent->ComponentTags.Contains(*AttributeName) || SceneCaptureComponent->TextureTarget == nullptr)
						{
							continue;
						}
						FTextureRenderTargetResource *TextureRenderTargetResource = SceneCaptureComponent->TextureTarget->GameThread_GetRenderTargetResource();
						TArray<FColor> ColorArray;
						FReadSurfaceDataFlags ReadSurfaceDataFlags;
						ReadSurfaceDataFlags.SetLinearToGamma(false);
						TextureRenderTargetResource->ReadPixels(ColorArray, ReadSurfaceDataFlags);

						const int DataSize = SceneCaptureComponent->TextureTarget->SizeX * SceneCaptureComponent->TextureTarget->SizeY;
						const int ExpectedDataSize = AttributeUint8DataMap.Contains(SendData.Value) ? AttributeUint8DataMap[SendData.Value].Num() / 3 : AttributeUint16DataMap[SendData.Value].Num();
						if (DataSize != ExpectedDataSize)
						{
							UE_LOG(LogMultiverseClient, Warning, TEXT("DataSize %d != ExpectedDataSize %d"), 3 * DataSize, ExpectedDataSize)
						}
						else
						{
							if (SendData.Value == EAttribute::RGB_3840_2160 || SendData.Value == EAttribute::RGB_1280_1024 || SendData.Value == EAttribute::RGB_640_480 || SendData.Value == EAttribute::RGB_128_128)
							{
								for (int i = 0; i < DataSize; i++)
								{
									*send_buffer_uint8_addr++ = ColorArray[i].R;
									*send_buffer_uint8_addr++ = ColorArray[i].G;
									*send_buffer_uint8_addr++ = ColorArray[i].B;
								}
							}
							else if (SendData.Value == EAttribute::Depth_3840_2160 || SendData.Value == EAttribute::Depth_1280_1024 || SendData.Value == EAttribute::Depth_640_480 || SendData.Value == EAttribute::Depth_128_128)
							{
								for (int i = 0; i < DataSize; i++)
								{
									*send_buffer_uint16_addr++ = ColorArray[i].R;
								}
							}
						}
					}
					break;
				}

				default:
					break;
				}
			}
		}
		else if (CachedBoneNames.Contains(SendData.Key))
		{
			switch (SendData.Value)
			{
			case EAttribute::JointRvalue:
			{
				for (TPair<UMultiverseAnim *, FName> &BoneNameMapping : CachedBoneNames[SendData.Key])
				{
					const FQuat JointQuaternion = BoneNameMapping.Key->JointPoses[BoneNameMapping.Value].GetRotation();
					*send_buffer_double_addr++ = FMath::RadiansToDegrees(JointQuaternion.GetAngle());
					break;
				}
				break;
			}

			case EAttribute::JointTvalue:
			{
				for (TPair<UMultiverseAnim *, FName> &BoneNameMapping : CachedBoneNames[SendData.Key])
				{
					const FVector JointPosition = BoneNameMapping.Key->JointPoses[BoneNameMapping.Value].GetTranslation();
					*send_buffer_double_addr++ = JointPosition.Y;
					break;
				}
				break;
			}

			default:
				break;
			}
		}
		else
		{
			if (CachedComponents.Contains(SendData.Key))
			{
				if (UCameraComponent *CameraComponent = Cast<UCameraComponent>(CachedComponents[SendData.Key]))
				{
					if (SendData.Value == EAttribute::Position)
					{
						const FVector CameraLocation = CameraComponent->GetComponentLocation();
						*send_buffer_double_addr++ = CameraLocation.X;
						*send_buffer_double_addr++ = CameraLocation.Y;
						*send_buffer_double_addr++ = CameraLocation.Z;
					}
					else if (SendData.Value == EAttribute::Quaternion)
					{
						const FQuat CameraQuat = CameraComponent->GetComponentQuat();
						*send_buffer_double_addr++ = CameraQuat.W;
						*send_buffer_double_addr++ = CameraQuat.X;
						*send_buffer_double_addr++ = CameraQuat.Y;
						*send_buffer_double_addr++ = CameraQuat.Z;
					}
				}
				else if (USkeletalMeshComponent *SkeletalMeshComponent = Cast<USkeletalMeshComponent>(CachedComponents[SendData.Key]))
				{
					if (SendData.Value == EAttribute::Position)
					{
						const FVector HandLocation = SkeletalMeshComponent->GetComponentLocation();
						*send_buffer_double_addr++ = HandLocation.X;
						*send_buffer_double_addr++ = HandLocation.Y;
						*send_buffer_double_addr++ = HandLocation.Z;
					}
					else if (SendData.Value == EAttribute::Quaternion)
					{
						const FQuat HandQuat = SkeletalMeshComponent->GetComponentQuat();
						*send_buffer_double_addr++ = HandQuat.W;
						*send_buffer_double_addr++ = HandQuat.X;
						*send_buffer_double_addr++ = HandQuat.Y;
						*send_buffer_double_addr++ = HandQuat.Z;
					}
				}
				else
				{
					UE_LOG(LogMultiverseClient, Error, TEXT("This should not happen"))
				}
			}

#ifdef WIN32
			APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
			for (const FString &Tag : {TEXT("LeftHand"), TEXT("RightHand")})
			{
				if (SendData.Key.Contains(Tag))
				{
					TArray<UActorComponent *> ActorComponents = PlayerPawn->GetComponentsByTag(UOculusXRHandComponent::StaticClass(), *Tag);
					if (ActorComponents.Num() != 1)
					{
						UE_LOG(LogMultiverseClient, Warning, TEXT("Found %d %s"), ActorComponents.Num(), *Tag)
						continue;
					}
					UOculusXRHandComponent *OculusXRHandComponent = Cast<UOculusXRHandComponent>(ActorComponents[0]);

					const EOculusXRBone *Bone = OculusXRHandComponent->BoneNameMappings.FindKey(*SendData.Key);
					if (Bone == nullptr)
					{
						UE_LOG(LogMultiverseClient, Warning, TEXT("Bone %s is nullptr"), *SendData.Key)
						continue;
					}
					const FString BoneName = UEnum::GetDisplayValueAsText(*Bone).ToString();
					if (SendData.Value == EAttribute::Position)
					{
						const FVector BoneLocation = OculusXRHandComponent->GetBoneLocationByName(*BoneName, EBoneSpaces::WorldSpace);
						*send_buffer_double_addr++ = BoneLocation.X;
						*send_buffer_double_addr++ = BoneLocation.Y;
						*send_buffer_double_addr++ = BoneLocation.Z;
					}
					else if (SendData.Value == EAttribute::Quaternion)
					{
						const FRotator BoneRotator = OculusXRHandComponent->GetBoneRotationByName(*BoneName, EBoneSpaces::WorldSpace);
						const FQuat BoneQuat = BoneRotator.Quaternion();
						*send_buffer_double_addr++ = BoneQuat.W;
						*send_buffer_double_addr++ = BoneQuat.X;
						*send_buffer_double_addr++ = BoneQuat.Y;
						*send_buffer_double_addr++ = BoneQuat.Z;
					}
				}
			}
#endif
		}
	}
}

void FMultiverseClient::bind_receive_data()
{
	double *receive_buffer_double_addr = receive_buffer.buffer_double.data;
	for (const TPair<FString, EAttribute> &ReceiveData : ReceiveDataArray)
	{
		if ((*ReceiveCustomObjectsPtr).Contains(ReceiveData.Key))
		{
			for (TPair<EAttribute, FDataContainer> &Attribute : (*ReceiveCustomObjectsPtr)[ReceiveData.Key].Attributes)
			{
				for (double &Data : Attribute.Value.Data)
				{
					Data = *receive_buffer_double_addr++;
				}
			}
		}
		if (CachedActors.Contains(ReceiveData.Key))
		{
			if (CachedActors[ReceiveData.Key] == nullptr)
			{
				UE_LOG(LogMultiverseClient, Warning, TEXT("Ignore None Object in CachedActors"))
				continue;
			}

			switch (ReceiveData.Value)
			{
			case EAttribute::Position:
			{
				const double X = *receive_buffer_double_addr++;
				const double Y = *receive_buffer_double_addr++;
				const double Z = *receive_buffer_double_addr++;
				CachedActors[ReceiveData.Key]->SetActorLocation(FVector(X, Y, Z));
				break;
			}

			case EAttribute::Quaternion:
			{
				const double W = *receive_buffer_double_addr++;
				const double X = *receive_buffer_double_addr++;
				const double Y = *receive_buffer_double_addr++;
				const double Z = *receive_buffer_double_addr++;
				CachedActors[ReceiveData.Key]->SetActorRotation(FQuat(X, Y, Z, W));
				break;
			}

			default:
				break;
			}
		}
		else if (CachedBoneNames.Contains(ReceiveData.Key))
		{
			const double Jointvalue = *receive_buffer_double_addr++;
			for (TPair<UMultiverseAnim *, FName> &BoneNameMapping : CachedBoneNames[ReceiveData.Key])
			{
				switch (ReceiveData.Value)
				{
				case EAttribute::JointRvalue:
				{
					BoneNameMapping.Key->JointPoses[BoneNameMapping.Value].SetRotation(FQuat(FRotator(Jointvalue, 0.f, 0.f)));
					break;
				}

				case EAttribute::JointTvalue:
				{
					BoneNameMapping.Key->JointPoses[BoneNameMapping.Value].SetTranslation(FVector(0.f, Jointvalue, 0.f));
					break;
				}

				default:
					break;
				}
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
	StartTime = FPlatformTime::Seconds();
}