#include "Utilities/CGFStatics.h"

bool UCGFStatics::IsItemInstanceValid(const FItemInstance& Instance)
{
	return Instance.IsValid();
}

FString UCGFStatics::GetItemInstanceDebugString(const FItemInstance& Instance)
{
	if (!Instance.IsValid())
	{
		return TEXT("[Invalid Item]");
	}

	return FString::Printf(TEXT("[%s] ID:%s Stack:%d Fragments:%d"),
		*Instance.ItemDefinitionId.ToString(),
		*Instance.InstanceId.ToString(),
		Instance.StackCount,
		Instance.InstanceFragments.Num());
}

bool UCGFStatics::ItemMatchesTagFilter(const FGameplayTagContainer& ItemTags,
	const FGameplayTagContainer& AllowedTags,
	const FGameplayTagContainer& DeniedTags)
{
	// If denied tags are specified and item has any, reject
	if (DeniedTags.Num() > 0 && ItemTags.HasAny(DeniedTags))
	{
		return false;
	}

	// If allowed tags are specified, item must have at least one
	if (AllowedTags.Num() > 0 && !ItemTags.HasAny(AllowedTags))
	{
		return false;
	}

	return true;
}

FGuid UCGFStatics::GenerateNewGuid()
{
	return FGuid::NewGuid();
}

bool UCGFStatics::IsGuidValid(const FGuid& Guid)
{
	return Guid.IsValid();
}
