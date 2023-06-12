// Copyright (c) 2022, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "Animation/AnimInstance.h"
// clang-format off
#include "USAnim.generated.h"
// clang-format on

UCLASS()
class USCONNECTOR_API UUSAnim : public UAnimInstance
{
	GENERATED_BODY()

public:
	UUSAnim();

public:
	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, FTransform> JointPoses;
};