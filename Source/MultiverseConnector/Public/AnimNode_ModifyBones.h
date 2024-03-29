// Copyright (c) 2022, Giang Hoang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "BonePose.h"
// clang-format off
#include "AnimNode_ModifyBones.generated.h"
// clang-format on

USTRUCT(BlueprintInternalUseOnly)
struct MULTIVERSECONNECTOR_API FAnimNode_ModifyBones : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = JointPoses, meta = (PinShownByDefault))
	TMap<FName, FTransform> JointPoses;

public:
	FAnimNode_ModifyBones();

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
	                                               TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	TArray<FBoneReference> BonesToModify;
};
