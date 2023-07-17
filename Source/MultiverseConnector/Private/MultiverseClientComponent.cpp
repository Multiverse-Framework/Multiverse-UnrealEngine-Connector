// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientComponent.h"

#include <chrono>

#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "Json.h"
#include "Math/UnrealMathUtility.h"
#include "MultiverseAnim.h"
#include "MultiverseClient.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "ZMQLibrary/zmq.hpp"

DEFINE_LOG_CATEGORY_STATIC(LogMultiverseClientComponent, Log, All);

const TMap<EAttribute, TArray<double>> AttributeMap =
    {
        {EAttribute::Position, {0.0, 0.0, 0.0}},
        {EAttribute::Quaternion, {1.0, 0.0, 0.0, 0.0}},
        {EAttribute::JointRvalue, {0.0}},
        {EAttribute::JointTvalue, {0.0}},
        {EAttribute::JointPosition, {0.0, 0.0, 0.0}},
        {EAttribute::JointQuaternion, {1.0, 0.0, 0.0, 0.0}}};

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

            CachedActors.Add(Object.Key->GetActorLabel(), Object.Key);
        }

        FString ObjectName = Object.Key->GetActorLabel();
        ObjectName.RemoveFromEnd(TEXT("_ref"));
        MetaDataJson->SetArrayField(ObjectName, AttributeJsonArray);
    }
    else if (ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(Object.Key))
    {
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
                        TArray<TSharedPtr<FJsonValue>> AttributeJsonArray = {MakeShareable(new FJsonValueString(TEXT("joint_rvalue")))};
                        CachedBoneNames.Add(BoneNameStr, TPair<UMultiverseAnim *, FName>(MultiverseAnim, BoneName));
                        MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
                    }
                    else if ((BoneNameStr.EndsWith(TEXT("_prismatic_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_prismatic_bone"))) &&
                             Object.Value.Attributes.Contains(EAttribute::JointTvalue))
                    {
                        TArray<TSharedPtr<FJsonValue>> AttributeJsonArray = {MakeShareable(new FJsonValueString(TEXT("joint_tvalue")))};
                        CachedBoneNames.Add(BoneNameStr, TPair<UMultiverseAnim *, FName>(MultiverseAnim, BoneName));
                        MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
                    }
                }
            }
            else
            {
                UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a MultiverseAnim."), *SkeletalMeshActor->GetActorLabel())
            }
        }
        else
        {
            UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a UStaticMeshComponent."), *SkeletalMeshActor->GetActorLabel())
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
            DataArray.Add(TPair<FString, EAttribute>(Object.Key->GetActorLabel(), Attribute));
        }
    }
    else if (ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(Object.Key))
    {
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
                UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a MultiverseAnim."), *SkeletalMeshActor->GetActorLabel())
            }
        }
        else
        {
            UE_LOG(LogMultiverseClientComponent, Warning, TEXT("SkeletalMeshActor %s does not contain a UStaticMeshComponent."), *SkeletalMeshActor->GetActorLabel())
        }

        DataArray.Sort([](const TPair<FString, EAttribute> &DataA, const TPair<FString, EAttribute> &DataB)
                       { return DataB.Key.Compare(DataA.Key) > 0; });
    }
}

UMultiverseClientComponent::UMultiverseClientComponent()
{
    Host = TEXT("tcp://127.0.0.1");
    Port = TEXT("7600");
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

UMaterial *UMultiverseClientComponent::GetMaterial(const FLinearColor &Color) const
{
    const FString ColorName = TEXT("M_") + ColorMap[Color];
    return Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *(TEXT("Material'/MultiverseConnector/Assets/Materials/") + ColorName + TEXT(".") + ColorName + TEXT("'"))));
}

bool UMultiverseClientComponent::init_objects()
{
    if (SendObjects.Num() > 0)
    {
        SendObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
                            { return ActorB.GetActorLabel().Compare(ActorA.GetActorLabel()) > 0; });
    }
    if (ReceiveObjects.Num() > 0)
    {
        ReceiveObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
                               { return ActorB.GetActorLabel().Compare(ActorA.GetActorLabel()) > 0; });
    }

    UWorld *World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogMultiverseClientComponent, Error, TEXT("World of %s is nullptr"), *GetName())
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
                UE_LOG(LogMultiverseClientComponent, Warning, TEXT("StaticMeshActor %s in ReceiveObjects has None StaticMeshComponent."), *ReceiveObject.Key->GetActorLabel())
                continue;
            }
            if (!StaticMeshComponent->IsSimulatingPhysics())
            {
                UE_LOG(LogMultiverseClientComponent, Warning, TEXT("StaticMeshActor %s has disabled physics, enabling physics."), *ReceiveObject.Key->GetActorLabel())
                StaticMeshComponent->SetSimulatePhysics(true);
            }

            UMaterial *RedMaterial = GetMaterial(FLinearColor(1, 0, 0, 1));
            for (int32 i = 0; i < StaticMeshComponent->GetMaterials().Num(); i++)
            {
                StaticMeshComponent->SetMaterial(i, RedMaterial);
            }

            FActorSpawnParameters SpawnParams;
            SpawnParams.Template = StaticMeshActor;
            SpawnParams.Name = *(ReceiveObject.Key->GetActorLabel() + TEXT("_ref"));
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AActor *ReceiveObjectRef = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(), SpawnParams);
            if (ReceiveObjectRef == nullptr)
            {
                UE_LOG(LogMultiverseClientComponent, Error, TEXT("Failed to spawn StaticMeshActor %s"), *SpawnParams.Name.ToString())
                continue;
            }

            ReceiveObjectRef->SetActorLabel(SpawnParams.Name.ToString());

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

void UMultiverseClientComponent::start_connect_to_server_thread()
{
    ConnectToServerTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
                                                                         { UMultiverseClientComponent::connect_to_server(); },
                                                                         TStatId(), nullptr, ENamedThreads::AnyThread);
}

void UMultiverseClientComponent::wait_for_connect_to_server_thread_finish()
{
    if (ConnectToServerTask.IsValid())
    {
        ConnectToServerTask->Wait();
    }
}

void UMultiverseClientComponent::start_meta_data_thread()
{
    MetaDataTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
                                                                  { UMultiverseClientComponent::send_and_receive_meta_data(); },
                                                                  TStatId(), nullptr, ENamedThreads::AnyThread);
}

void UMultiverseClientComponent::wait_for_meta_data_thread_finish()
{
    if (MetaDataTask.IsValid())
    {
        MetaDataTask->Wait();
    }
}

void UMultiverseClientComponent::bind_send_meta_data()
{
    TSharedPtr<FJsonObject> SendMetaDataJson = MakeShareable(new FJsonObject);
    SendMetaDataJson->SetStringField("world", "world");
    SendMetaDataJson->SetStringField("time_unit", "s");
    SendMetaDataJson->SetStringField("simulator", "unreal");
    SendMetaDataJson->SetStringField("length_unit", "cm");
    SendMetaDataJson->SetStringField("angle_unit", "deg");
    SendMetaDataJson->SetStringField("handedness", "lhs");
    SendMetaDataJson->SetStringField("force_unit", "N");

    SendMetaDataJson->SetObjectField("send", MakeShareable(new FJsonObject));
    SendMetaDataJson->SetObjectField("receive", MakeShareable(new FJsonObject));

    for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
    {
        if (SendObject.Key == nullptr)
        {
            UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in SendObjects"))
            continue;
        }

        BindMetaData(SendMetaDataJson->GetObjectField("send"), SendObject, CachedActors, CachedBoneNames);
    }

    for (const TPair<AActor *, FAttributeContainer> &ReceiveObjectRef : ReceiveObjectRefs)
    {
        if (ReceiveObjectRef.Key == nullptr)
        {
            UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in ReceiveObjectRefs"))
            continue;
        }

        BindMetaData(SendMetaDataJson->GetObjectField("receive"), ReceiveObjectRef, CachedActors, CachedBoneNames);
    }

    FString SendMetaDataString;
    TSharedRef<TJsonWriter<TCHAR>> Writer = TJsonWriterFactory<TCHAR>::Create(&SendMetaDataString);
    FJsonSerializer::Serialize(SendMetaDataJson.ToSharedRef(), Writer, true);

    std::string send_meta_data_string;
    for (size_t i = 0; i < (SendMetaDataString.Len() + 127) / 128; i++) // Split string into multiple substrings with a length of 128 characters or less
    {
        const int32 StartIndex = i * 128;
        const int32 SubstringLength = FMath::Min(128, SendMetaDataString.Len() - StartIndex);
        const FString Substring = SendMetaDataString.Mid(StartIndex, SubstringLength);
        send_meta_data_string += StringCast<ANSICHAR>(*Substring).Get();
    }

    UE_LOG(LogMultiverseClientComponent, Log, TEXT("%s"), *SendMetaDataString)

    // reader.parse(send_meta_data_string, send_meta_data_json);
}

void UMultiverseClientComponent::bind_receive_meta_data()
{
    // for (const TPair<FString, EAttribute> &SendData : SendDataArray)
    // {
    //     const std::string object_name = StringCast<ANSICHAR>(*SendData.Key).Get();

    //     if (CachedActors.Contains(SendData.Key))
    //     {
    //         if (CachedActors[SendData.Key] == nullptr)
    //         {
    //             UE_LOG(LogMultiverseClientComponent, Warning, TEXT("Ignore None Object in CachedActors"))
    //             continue;
    //         }

    //         switch (SendData.Value)
    //         {
    //         case EAttribute::Position:
    //         {
    //             const double X = receive_meta_data_json["send"][object_name]["position"][0].asDouble();
    //             const double Y = receive_meta_data_json["send"][object_name]["position"][1].asDouble();
    //             const double Z = receive_meta_data_json["send"][object_name]["position"][2].asDouble();
    //             CachedActors[SendData.Key]->SetActorLocation(FVector(X, Y, Z));
    //             break;
    //         }

    //         case EAttribute::Quaternion:
    //         {
    //             const double W = receive_meta_data_json["send"][object_name]["quaternion"][0].asDouble();
    //             const double X = receive_meta_data_json["send"][object_name]["quaternion"][1].asDouble();
    //             const double Y = receive_meta_data_json["send"][object_name]["quaternion"][2].asDouble();
    //             const double Z = receive_meta_data_json["send"][object_name]["quaternion"][3].asDouble();
    //             CachedActors[SendData.Key]->SetActorRotation(FQuat(X, Y, Z, W));
    //             break;
    //         }

    //         default:
    //             break;
    //         }
    //     }
    //     else if (CachedBoneNames.Contains(SendData.Key))
    //     {
    //         switch (SendData.Value)
    //         {
    //         case EAttribute::JointRvalue:
    //         {
    //             const double JointRvalue = receive_meta_data_json["send"][object_name]["joint_rvalue"][0].asDouble();
    //             CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].SetRotation(FQuat(FRotator(JointRvalue, 0.f, 0.f)));
    //             break;
    //         }

    //         case EAttribute::JointTvalue:
    //         {
    //             const double JointTvalue = receive_meta_data_json["send"][object_name]["joint_tvalue"][0].asDouble();
    //             CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].SetTranslation(FVector(0.f, JointTvalue, 0.f));
    //             break;
    //         }

    //         default:
    //             break;
    //         }
    //     }
    // }
}

void UMultiverseClientComponent::init_send_and_receive_data()
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

void UMultiverseClientComponent::bind_send_data()
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

void UMultiverseClientComponent::bind_receive_data()
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

void UMultiverseClientComponent::clean_up()
{
    SendDataArray.Empty();

    ReceiveDataArray.Empty();
}