// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientComponent.h"

#include "MultiverseClient.h"

UMultiverseClientComponent::UMultiverseClientComponent()
{
    Host = TEXT("tcp://127.0.0.1");
    Port = TEXT("7600");
}

void UMultiverseClientComponent::Init()
{   
    MultiverseClient.Init(Host, Port, SendObjects, ReceiveObjects, GetWorld());

    MultiverseClient.connect();
}

void UMultiverseClientComponent::Tick()
{   
    MultiverseClient.communicate();
}

void UMultiverseClientComponent::Deinit()
{   
    MultiverseClient.disconnect();
}