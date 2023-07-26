// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientComponent.h"

#include "MultiverseClient.h"

UMultiverseClientComponent::UMultiverseClientComponent()
{
    Host = TEXT("tcp://127.0.0.1");
    ServerPort = TEXT("7000");
    ClientPort = TEXT("7600");
}

void UMultiverseClientComponent::Init()
{   
    MultiverseClient.Init(Host, ServerPort, ClientPort, SendObjects, ReceiveObjects, GetWorld());
}

void UMultiverseClientComponent::Tick()
{   
    MultiverseClient.communicate();
}

void UMultiverseClientComponent::Deinit()
{   
    MultiverseClient.disconnect();
}