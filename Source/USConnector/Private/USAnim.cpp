// Copyright (c) 2022, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "USAnim.h"

UUSAnim::UUSAnim()
{
}

void UUSAnim::NativeBeginPlay()
{
	TArray<FName> BoneNames;
	GetSkelMeshComponent()->GetBoneNames(BoneNames);
	for (const FName &BoneName : BoneNames)
	{
		if (BoneName.ToString().EndsWith(TEXT("continuous_bone"), ESearchCase::CaseSensitive) ||
			BoneName.ToString().EndsWith(TEXT("prismatic_bone"), ESearchCase::CaseSensitive) ||
			BoneName.ToString().EndsWith(TEXT("revolute_bone"), ESearchCase::CaseSensitive) ||
			BoneName.ToString().EndsWith(TEXT("ball_bone"), ESearchCase::CaseSensitive))
		{
			JointPoses.Add(BoneName);
		}
	}
}

void UUSAnim::NativeUpdateAnimation(float DeltaSeconds)
{
}