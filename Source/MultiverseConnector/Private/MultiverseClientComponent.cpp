// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientComponent.h"

#include "MultiverseClient.h"

UMultiverseClientComponent::UMultiverseClientComponent()
{

}

void UMultiverseClientComponent::Init()
{   
    MultiverseClient.Init(ServerHost, ServerPort, ClientHost, ClientPort, WorldName, SimulationName, SendObjects, ReceiveObjects, GetWorld());
}

void UMultiverseClientComponent::Tick()
{
    MultiverseClient.communicate();
}

void UMultiverseClientComponent::Deinit()
{
    MultiverseClient.disconnect();
}