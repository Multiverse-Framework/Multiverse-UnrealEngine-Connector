// Copyright (c) 2022, Giang Hoang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "Animation/AnimInstance.h"
// clang-format off
#include "MultiverseAnim.generated.h"
// clang-format on

UCLASS()
class MULTIVERSECONNECTOR_API UMultiverseAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	UMultiverseAnim();

public:
	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FTransform> JointPoses;
};
