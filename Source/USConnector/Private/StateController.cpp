// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "StateController.h"

#include <chrono>

#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "Json.h"
#include "Math/UnrealMathUtility.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "StateManager.h"
#include "USAnim.h"
#include "ZMQLibrary/zmq.hpp"

DEFINE_LOG_CATEGORY_STATIC(LogStateController, Log, All);

const TMap<EAttribute, TArray<double>> AttributeMap =
    {
        {EAttribute::Position, {0.0, 0.0, 0.0}},
        {EAttribute::Quaternion, {1.0, 0.0, 0.0, 0.0}},
        {EAttribute::JointRvalue, {0.0}},
        {EAttribute::JointTvalue, {0.0}},
        {EAttribute::JointPosition, {0.0, 0.0, 0.0}},
        {EAttribute::JointQuaternion, {1.0, 0.0, 0.0, 0.0}}};

static void SetMetaData(TArray<TPair<FString, EAttribute>> &DataArray,
                        size_t &buffer_size, TSharedPtr<FJsonObject> &MetaDataJson,
                        const TPair<AActor *, FAttributeContainer> &Object,
                        TMap<FString, AActor *> &CachedActors,
                        TMap<FString, TPair<UUSAnim *, FName>> &CachedBoneNames)
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

            DataArray.Add(TPair<FString, EAttribute>(Object.Key->GetActorLabel(), Attribute));
            CachedActors.Add(Object.Key->GetActorLabel(), Object.Key);
            buffer_size += AttributeMap[Attribute].Num();
        }

        FString ObjectName = Object.Key->GetActorLabel();
        ObjectName.RemoveFromEnd(TEXT("_ref"));
        MetaDataJson->SetArrayField(ObjectName, AttributeJsonArray);
    }
    else if (ASkeletalMeshActor *SkeletalMeshActor = Cast<ASkeletalMeshActor>(Object.Key))
    {
        if (USkeletalMeshComponent *SkeletalMeshComponent = SkeletalMeshActor->GetSkeletalMeshComponent())
        {
            if (UUSAnim *USAnim = Cast<UUSAnim>(SkeletalMeshComponent->GetAnimInstance()))
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
                        CachedBoneNames.Add(BoneNameStr, TPair<UUSAnim *, FName>(USAnim, BoneName));
                        DataArray.Add(TPair<FString, EAttribute>(BoneNameStr, EAttribute::JointRvalue));
                        buffer_size += AttributeMap[EAttribute::JointRvalue].Num();
                        MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
                    }
                    else if ((BoneNameStr.EndsWith(TEXT("_prismatic_bone")) && BoneNameStr.RemoveFromEnd(TEXT("_prismatic_bone"))) &&
                             Object.Value.Attributes.Contains(EAttribute::JointTvalue))
                    {
                        TArray<TSharedPtr<FJsonValue>> AttributeJsonArray = {MakeShareable(new FJsonValueString(TEXT("joint_tvalue")))};
                        CachedBoneNames.Add(BoneNameStr, TPair<UUSAnim *, FName>(USAnim, BoneName));
                        DataArray.Add(TPair<FString, EAttribute>(BoneNameStr, EAttribute::JointTvalue));
                        buffer_size += AttributeMap[EAttribute::JointTvalue].Num();
                        MetaDataJson->SetArrayField(BoneNameStr, AttributeJsonArray);
                    }
                }
            }
            else
            {
                UE_LOG(LogStateController, Warning, TEXT("SkeletalMeshActor %s does not contain a USAnim."), *SkeletalMeshActor->GetName())
            }
        }
        else
        {
            UE_LOG(LogStateController, Warning, TEXT("SkeletalMeshActor %s does not contain a UStaticMeshComponent."), *SkeletalMeshActor->GetName())
        }

        DataArray.Sort([](const TPair<FString, EAttribute> &DataA, const TPair<FString, EAttribute> &DataB)
                          { return DataB.Key.Compare(DataA.Key) > 0; });
    }
}

UStateController::UStateController()
{
    Host = TEXT("tcp://127.0.0.1");
    Port = 7500;
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

UMaterial *UStateController::GetMaterial(const FLinearColor &Color) const
{
    const FString ColorName = TEXT("M_") + ColorMap[Color];
    return Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *(TEXT("Material'/USConnector/Assets/Materials/") + ColorName + TEXT(".") + ColorName + TEXT("'"))));
}

void UStateController::Init()
{
    if (SendObjects.Num() > 0)
    {
        SendObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
                            { return ActorB.GetName().Compare(ActorA.GetName()) > 0; });
    }
    if (ReceiveObjects.Num() > 0)
    {
        ReceiveObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
                               { return ActorB.GetName().Compare(ActorA.GetName()) > 0; });
    }

    UWorld *World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogStateController, Error, TEXT("World of %s is nullptr"), *GetName())
        return;
    }

    for (const TPair<AActor *, FAttributeContainer> &ReceiveObject : ReceiveObjects)
    {
        if (ReceiveObject.Key == nullptr)
        {
            UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in ReceiveObjects."))
            continue;
        }

        if (AStaticMeshActor *StaticMeshActor = Cast<AStaticMeshActor>(ReceiveObject.Key))
        {
            UStaticMeshComponent *StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
            if (StaticMeshComponent == nullptr || StaticMeshComponent->GetStaticMesh() == nullptr)
            {
                UE_LOG(LogStateController, Warning, TEXT("StaticMeshActor %s in ReceiveObjects has None StaticMeshComponent."), *ReceiveObject.Key->GetName())
                continue;
            }
            if (!StaticMeshComponent->IsSimulatingPhysics())
            {
                UE_LOG(LogStateController, Warning, TEXT("StaticMeshActor %s has disabled physics, enabling physics."), *ReceiveObject.Key->GetName())
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
                UE_LOG(LogStateController, Error, TEXT("Failed to spawn StaticMeshActor %s"), *SpawnParams.Name.ToString())
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

    if (SendObjects.Num() > 0 || ReceiveObjectRefs.Num() > 0)
    {
        UE_LOG(LogStateController, Log, TEXT("Initializing the socket connection..."))

        context = zmq_ctx_new();

        socket_client = zmq_socket(context, ZMQ_REQ);
        SocketAddr = Host + ":" + FString::FromInt(Port);
        socket_addr = StringCast<ANSICHAR>(*SocketAddr).Get();

        SendMetaData();
    }
}

void UStateController::SendMetaData()
{
    zmq_disconnect(socket_client, socket_addr.c_str());
    zmq_connect(socket_client, socket_addr.c_str());

    SendDataArray.Empty();
    ReceiveDataArray.Empty();

    Task = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
                                                          { 
        TSharedPtr<FJsonObject> MetaDataJson = MakeShareable(new FJsonObject);
        MetaDataJson->SetStringField("time", "microseconds");
        MetaDataJson->SetStringField("simulator", "unreal");
        MetaDataJson->SetStringField("length_unit", "cm");
        MetaDataJson->SetStringField("angle_unit", "deg");
        MetaDataJson->SetStringField("handedness", "lhs");
        MetaDataJson->SetStringField("force_unit", "N");

        TSharedPtr<FJsonObject> MetaDataSendJson = MakeShareable(new FJsonObject);
        MetaDataJson->SetObjectField("send", MetaDataSendJson);

        send_buffer_size = 1;

        for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
        {
            if (SendObject.Key == nullptr)
            {
                UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in SendObjects"))
                continue;
            }

            SetMetaData(SendDataArray, send_buffer_size, MetaDataSendJson, SendObject, CachedActors, CachedBoneNames);
        }

        TSharedPtr<FJsonObject> MetaDataReceiveJson = MakeShareable(new FJsonObject);
        MetaDataJson->SetObjectField("receive", MetaDataReceiveJson);

        receive_buffer_size = 1;

        for (const TPair<AActor *, FAttributeContainer> &ReceiveObjectRef : ReceiveObjectRefs)
        {
            if (ReceiveObjectRef.Key == nullptr)
            {
                UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in ReceiveObjectRefs"))
                continue;
            }

            SetMetaData(ReceiveDataArray, receive_buffer_size, MetaDataReceiveJson, ReceiveObjectRef, CachedActors, CachedBoneNames);
        }

        FString MetaDataString;
        TSharedRef<TJsonWriter<TCHAR>> Writer = TJsonWriterFactory<TCHAR>::Create(&MetaDataString);
        FJsonSerializer::Serialize(MetaDataJson.ToSharedRef(), Writer, true);

		double *buffer = (double *)calloc(send_buffer_size + 2, sizeof(double));
        std::string meta_data_string;
        for (size_t i = 0; i < (MetaDataString.Len() + 127) / 128; i++) // Split string into multiple substrings with a length of 128 characters or less
        {
            const int32 StartIndex = i * 128;
            const int32 SubstringLength = FMath::Min(128, MetaDataString.Len() - StartIndex);
            const FString Substring = MetaDataString.Mid(StartIndex, SubstringLength);
            meta_data_string += StringCast<ANSICHAR>(*Substring).Get();
        }

        UE_LOG(LogStateController, Log, TEXT("%s"), *MetaDataString)
        
		while (true)
		{
			// Send JSON string over ZMQ
			zmq_send(socket_client, meta_data_string.c_str(), meta_data_string.size(), 0);

			// Receive buffer sizes and send_data (if exists) over ZMQ
			zmq_recv(socket_client, buffer, (send_buffer_size + 2) * sizeof(double), 0);
			if (*buffer < 0)
			{
				free(buffer);
				buffer = (double *)calloc(send_buffer_size + 2, sizeof(double));
				UE_LOG(LogStateController, Warning, TEXT("The socket server at %s has been terminated, resend the message"), *SocketAddr);
				zmq_disconnect(socket_client, socket_addr.c_str());
                zmq_connect(socket_client, socket_addr.c_str());
			}
			else
			{
				break;
			}
		}
        
        size_t recv_buffer_size[2] = {(size_t)buffer[0], (size_t)buffer[1]};
        if (recv_buffer_size[0] != send_buffer_size || recv_buffer_size[1] != receive_buffer_size)
        {
            UE_LOG(LogStateController, Error, TEXT("Failed to initialize the socket at %s: send_buffer_size(server = %ld != client = %ld), receive_buffer_size(server = %ld != client = %ld)."), *SocketAddr, recv_buffer_size[0], send_buffer_size, recv_buffer_size[1], receive_buffer_size)
            zmq_disconnect(socket_client, socket_addr.c_str());
        }
        else
        {
            if (buffer[2] < 0.0)
            {
                UE_LOG(LogStateController, Log, TEXT("Continue state on socket %s"), *SocketAddr)

                double *buffer_addr = buffer + 3;

                for (const TPair<FString, EAttribute> &SendData : SendDataArray)
                {
                    if (CachedActors.Contains(SendData.Key))
                    {
                        if (CachedActors[SendData.Key] == nullptr)
                        {
                            UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in CachedActors"))
                            continue;
                        }

                        switch (SendData.Value)
                        {
                        case EAttribute::Position:
                        {
                            const double X = *buffer_addr++;
                            const double Y = *buffer_addr++;
                            const double Z = *buffer_addr++;
                            CachedActors[SendData.Key]->SetActorLocation(FVector(X, Y, Z));
                            break;
                        }

                        case EAttribute::Quaternion:
                        {
                            const double W = *buffer_addr++;
                            const double X = *buffer_addr++;
                            const double Y = *buffer_addr++;
                            const double Z = *buffer_addr++;
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
                            const double JointRvalue = *buffer_addr++;
                            CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].SetRotation(FQuat(FRotator(JointRvalue, 0.f, 0.f)));
                            break;
                        }

                        case EAttribute::JointTvalue:
                        {
                            const double JointTvalue = *buffer_addr++;
                            const double Scale = CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].GetScale3D().Y;
                            CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].SetTranslation(FVector(0.f, JointTvalue / Scale, 0.f));
                            break;
                        }

                        default:
                            break;
                        }
                    }
                }
            }

            UE_LOG(LogStateController, Log, TEXT("Initialized the socket at %s successfully."), *SocketAddr)
            UE_LOG(LogStateController, Log, TEXT("Start communication on %s (send: %ld, receive: %ld)"), *SocketAddr, send_buffer_size, receive_buffer_size)
            send_buffer = (double *)calloc(send_buffer_size, sizeof(double));
            receive_buffer = (double *)calloc(receive_buffer_size, sizeof(double));
            IsEnable = true;
        } },
                                                          TStatId(), nullptr, ENamedThreads::AnyThread);
}

void UStateController::Tick()
{
    if (IsEnable)
    {
        *send_buffer = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

        double *send_buffer_addr = send_buffer + 1;
        for (const TPair<FString, EAttribute> &SendData : SendDataArray)
        {
            if (CachedActors.Contains(SendData.Key))
            {
                if (CachedActors[SendData.Key] == nullptr)
                {
                    UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in CachedActors"))
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
                    const double Scale = CachedBoneNames[SendData.Key].Key->JointPoses[CachedBoneNames[SendData.Key].Value].GetScale3D().Y;
                    *send_buffer_addr++ = JointPosition.Y * Scale;
                    break;
                }

                default:
                    break;
                }
            }
        }

        zmq_send(socket_client, send_buffer, send_buffer_size * sizeof(double), 0);

        zmq_recv(socket_client, receive_buffer, receive_buffer_size * sizeof(double), 0);

        if (*receive_buffer < 0)
        {
            IsEnable = false;
            UE_LOG(LogStateController, Warning, TEXT("The socket server at %s has been terminated, resend the message"), *SocketAddr)
            if (Task.IsValid())
            {
                Task->Wait();
            }
            SendMetaData();
            return;
        }

        double *receive_buffer_addr = receive_buffer + 1;
        for (const TPair<FString, EAttribute> &ReceiveData : ReceiveDataArray)
        {
            if (CachedActors.Contains(ReceiveData.Key))
            {
                if (CachedActors[ReceiveData.Key] == nullptr)
                {
                    UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in CachedActors"))
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
                    const double Scale = CachedBoneNames[ReceiveData.Key].Key->JointPoses[CachedBoneNames[ReceiveData.Key].Value].GetScale3D().Length();
                    UE_LOG(LogStateController, Log, TEXT("%s - %d"), *CachedBoneNames[ReceiveData.Key].Value.ToString(), Scale)
                    CachedBoneNames[ReceiveData.Key].Key->JointPoses[CachedBoneNames[ReceiveData.Key].Value].SetTranslation(FVector(0.f, JointTvalue / Scale, 0.f));
                    break;
                }

                default:
                    break;
                }
            }
        }
    }
}

void UStateController::Deinit()
{
    UE_LOG(LogStateController, Log, TEXT("Closing the socket client on %s"), *SocketAddr);
    if (IsEnable)
    {
        const std::string close_data = "{}";

        zmq_send(socket_client, close_data.c_str(), close_data.size(), 0);

        free(send_buffer);
        free(receive_buffer);

        zmq_disconnect(socket_client, socket_addr.c_str());
    }
    else if (Task.IsValid())
    {
        zmq_ctx_shutdown(context);
        Task->Wait();
    }
}