// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientComponent.h"

#include "MultiverseClient.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiverseClientComponent, Log, All);

UMultiverseClientComponent::UMultiverseClientComponent()
{

}

void UMultiverseClientComponent::Init()
{   
    if (!ServerHost.StartsWith("tcp://"))
    {
        ServerHost = FPaths::ProjectDir() / ServerHost;
        UE_LOG(LogMultiverseClientComponent, Log, TEXT("Read ServerHost from: %s"), *ServerHost)
        if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ServerHost))
        {
            UE_LOG(LogMultiverseClientComponent, Error, TEXT("ServerHost.txt file not found: %s"), *ServerHost)
            return;
        }
        if (!FFileHelper::LoadFileToString(ServerHost, *ServerHost))
        {
            UE_LOG(LogMultiverseClientComponent, Error, TEXT("Failed to load ServerHost.txt file: %s"), *ServerHost)
            return;
        }
    }
    UE_LOG(LogMultiverseClientComponent, Log, TEXT("ServerHost: %s"), *ServerHost)

    if (!ServerPort.IsNumeric())
    {
        ServerPort = FPaths::ProjectDir() / ServerPort;
        UE_LOG(LogMultiverseClientComponent, Log, TEXT("Read ServerPort from: %s"), *ServerPort)
        if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ServerPort))
        {
            UE_LOG(LogMultiverseClientComponent, Error, TEXT("ServerPort.txt file not found: %s"), *ServerPort)
            return;
        }
        if (!FFileHelper::LoadFileToString(ServerPort, *ServerPort))
        {
            UE_LOG(LogMultiverseClientComponent, Error, TEXT("Failed to load ServerPort.txt file: %s"), *ServerPort)
            return;
        }
    }
    UE_LOG(LogMultiverseClientComponent, Log, TEXT("ServerPort: %s"), *ServerPort)

    if (!ClientPort.IsNumeric())
    {
        ClientPort = FPaths::ProjectDir() / ClientPort;
        UE_LOG(LogMultiverseClientComponent, Log, TEXT("Read ClientPort from: %s"), *ClientPort)
        if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ClientPort))
        {
            UE_LOG(LogMultiverseClientComponent, Error, TEXT("ClientPort.txt file not found: %s"), *ClientPort)
            return;
        }
        if (!FFileHelper::LoadFileToString(ClientPort, *ClientPort))
        {
            UE_LOG(LogMultiverseClientComponent, Error, TEXT("Failed to load ClientPort.txt file: %s"), *ClientPort)
            return;
        }
    }
    UE_LOG(LogMultiverseClientComponent, Log, TEXT("ClientPort: %s"), *ClientPort)
    
    MultiverseClient.Init(ServerHost, ServerPort, ClientPort, WorldName, SimulationName, SendObjects, ReceiveObjects, GetWorld());
}

void UMultiverseClientComponent::Tick(float DeltaTime)
{
    CurrentCycleTime += DeltaTime;
    CurrentSimulationApiCycleTime += DeltaTime;
    if (SimulationApiCallbacks.Num() > 0 && bSimulationApiCallbacksEnabled && CurrentSimulationApiCycleTime >= 1.f / SimulationApiCallbacksRate)
    {
        SimulationApiCallbacksResponse = MultiverseClient.CallApis(SimulationApiCallbacks);
    }
    
    if (CurrentCycleTime >= 1.f / UpdateRate || CurrentSimulationApiCycleTime >= 1.f / SimulationApiCallbacksRate)
    {
        MultiverseClient.communicate();
    }

    if (CurrentCycleTime >= 1.f / UpdateRate)
    {
        CurrentCycleTime = 0.f;
    }
    if (CurrentSimulationApiCycleTime >= 1.f / SimulationApiCallbacksRate)
    {
        CurrentSimulationApiCycleTime = 0.f;
    }
}

void UMultiverseClientComponent::Deinit()
{
    MultiverseClient.disconnect();
}