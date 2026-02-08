#include "Types/CGFItemTypes.h"

// ---------------------------------------------------------------------------
// UItemInstanceFragment
// ---------------------------------------------------------------------------

TSharedPtr<FJsonObject> UItemInstanceFragment::SerializeFragment() const
{
	return MakeShared<FJsonObject>();
}

void UItemInstanceFragment::DeserializeFragment(const TSharedPtr<FJsonObject>& Data)
{
}

FText UItemInstanceFragment::GetFragmentDisplayText_Implementation() const
{
	return FText::GetEmpty();
}

bool UItemInstanceFragment::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	return true;
}

void UItemInstanceFragment::OnAttachedToInstance(const FItemInstance& OwningInstance)
{
}

// ---------------------------------------------------------------------------
// FItemInstance
// ---------------------------------------------------------------------------

bool FItemInstance::IsValid() const
{
	return InstanceId.IsValid() && ItemDefinitionId.IsValid() && StackCount > 0;
}

bool FItemInstance::operator==(const FItemInstance& Other) const
{
	return InstanceId == Other.InstanceId;
}

bool FItemInstance::operator!=(const FItemInstance& Other) const
{
	return !(*this == Other);
}

bool FItemInstance::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Ar << InstanceId;

	FString AssetIdString;
	if (Ar.IsSaving())
	{
		AssetIdString = ItemDefinitionId.ToString();
	}
	Ar << AssetIdString;
	if (Ar.IsLoading())
	{
		ItemDefinitionId = FPrimaryAssetId(AssetIdString);
	}

	Ar << StackCount;

	// Fragment serialization — count then each fragment's class and data
	int32 FragmentCount = InstanceFragments.Num();
	Ar << FragmentCount;

	if (Ar.IsLoading())
	{
		InstanceFragments.SetNum(FragmentCount);
		for (int32 i = 0; i < FragmentCount; ++i)
		{
			FString ClassName;
			Ar << ClassName;

			UClass* FragClass = FindObject<UClass>(nullptr, *ClassName);
			if (FragClass)
			{
				UItemInstanceFragment* Fragment = NewObject<UItemInstanceFragment>(GetTransientPackage(), FragClass);
				Fragment->NetSerialize(Ar, Map, bOutSuccess);
				InstanceFragments[i] = Fragment;
			}
		}
	}
	else
	{
		for (int32 i = 0; i < FragmentCount; ++i)
		{
			FString ClassName;
			if (InstanceFragments[i])
			{
				ClassName = InstanceFragments[i]->GetClass()->GetPathName();
			}
			Ar << ClassName;

			if (InstanceFragments[i])
			{
				InstanceFragments[i]->NetSerialize(Ar, Map, bOutSuccess);
			}
		}
	}

	bOutSuccess = true;
	return true;
}

// ---------------------------------------------------------------------------
// FInventorySlot — replication callbacks
// ---------------------------------------------------------------------------

void FInventorySlot::PreReplicatedRemove(const FInventorySlotArray& ArraySerializer)
{
	if (ArraySerializer.OwningComponent)
	{
		// Owning component will handle OnItemRemoved delegate dispatch
	}
}

void FInventorySlot::PostReplicatedAdd(const FInventorySlotArray& ArraySerializer)
{
	if (ArraySerializer.OwningComponent)
	{
		// Owning component will handle OnItemAdded delegate dispatch
	}
}

void FInventorySlot::PostReplicatedChange(const FInventorySlotArray& ArraySerializer)
{
	if (ArraySerializer.OwningComponent)
	{
		// Owning component will handle OnItemUpdated delegate dispatch
	}
}
