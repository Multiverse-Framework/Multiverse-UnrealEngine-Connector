// Copyright (c) 2023, Giang Hoang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientComponent.h"

UMultiverseClientComponent::UMultiverseClientComponent()
{
	
}

void UMultiverseClientComponent::Init()
{
	MultiverseClient.Init(ServerHost, ServerPort,
	                      ClientHost, ClientPort,
	                      WorldName, SimulationName, SendObjects, ReceiveObjects, GetWorld());
}

void UMultiverseClientComponent::Tick()
{
	MultiverseClient.communicate();
}

void UMultiverseClientComponent::Deinit()
{
	MultiverseClient.disconnect();
}
