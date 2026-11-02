// Copyright Daniel Raquel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

/**
 * Test-only helpers for the game framework plugins' automation tests (code review CR2 row 18, D-CR23 / FW-34). Compiled
 * only with WITH_AUTOMATION_TESTS.
 */
namespace CGFTesting
{
	/**
	 * A standalone game instance with its game world: the game-instance subsystems (the item database) and the world
	 * subsystems (loot generation) live, and an actor spawned in the world reaches them through GetWorld()->GetGameInstance()
	 * as a component does at runtime. Play has not begun: a component's BeginPlay does not run (allocate what it would).
	 */
	struct FTestGameInstance
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;

		FTestGameInstance()
		{
			GameInstance = NewObject<UGameInstance>(GEngine);
			GameInstance->AddToRoot();
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
		}

		FTestGameInstance(const FTestGameInstance&) = delete;
		FTestGameInstance& operator=(const FTestGameInstance&) = delete;

		~FTestGameInstance()
		{
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
			if (GameInstance)
			{
				GameInstance->Shutdown();
				GameInstance->RemoveFromRoot();
			}
		}

		/** A plain actor in the world (standalone, so it has authority), the owner for the components under test. */
		AActor* SpawnOwner() const
		{
			return World->SpawnActor<AActor>();
		}

		/**
		 * A plain actor as a client sees its own pawn (role AutonomousProxy, no authority): the owner for the client half of
		 * a simulated server/client test. Its Try* calls take their client branch; a Server RPC it sends is absorbed (the
		 * world is standalone), so the test delivers the request to the server half itself with FRPCCall.
		 */
		AActor* SpawnRemoteOwner() const
		{
			AActor* Owner = World->SpawnActor<AActor>();
			Owner->SetRole(ROLE_AutonomousProxy);
			return Owner;
		}

		/** A game-instance subsystem. */
		template <typename T>
		T* GetSubsystem() const
		{
			return GameInstance->GetSubsystem<T>();
		}

		/** A world subsystem. */
		template <typename T>
		T* GetWorldSubsystem() const
		{
			return World->GetSubsystem<T>();
		}
	};

	/**
	 * Deliver an RPC by name, as the net driver does on the receiving machine: the parameters are set by name and the
	 * function runs through ProcessEvent. A standalone world runs a Server RPC locally when its owner has authority and a
	 * Client RPC locally on any owner, so one world plays both machines of a simulated server/client test. Reflection
	 * ignores C++ access, so private RPCs are reachable.
	 *
	 *     FRPCCall(Server, TEXT("ServerRPC_RequestEquipFromInventory")).Set(TEXT("ItemInstanceId"), Id).Send();
	 */
	class FRPCCall
	{
	public:
		FRPCCall(UObject* InTarget, FName FunctionName)
			: Target(InTarget)
			, Function(InTarget ? InTarget->FindFunction(FunctionName) : nullptr)
		{
			// An RPC with no parameters has no buffer to initialize (InitializeStruct asserts on a null one).
			if (Function && Function->ParmsSize > 0)
			{
				Params.SetNumZeroed(Function->ParmsSize);
				Function->InitializeStruct(Params.GetData());
			}
		}

		~FRPCCall()
		{
			if (Function && Params.Num() > 0)
			{
				Function->DestroyStruct(Params.GetData());
			}
		}

		FRPCCall(const FRPCCall&) = delete;
		FRPCCall& operator=(const FRPCCall&) = delete;

		/**
		 * Set one parameter. T must be the parameter's C++ type (a size mismatch marks the call invalid).
		 * @param ParamName The parameter's name in the RPC's declaration
		 * @param Value     Its value
		 * @return This call, to chain the next parameter
		 */
		template <typename T>
		FRPCCall& Set(FName ParamName, const T& Value)
		{
			FProperty* Property = Function ? FindFProperty<FProperty>(Function, ParamName) : nullptr;
			if (!Property || Property->GetElementSize() != static_cast<int32>(sizeof(T)))
			{
				bValid = false;
				return *this;
			}
			Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Params.GetData()), &Value);
			return *this;
		}

		/**
		 * Run the RPC.
		 * @return False when the function or a parameter was not found (nothing ran)
		 */
		bool Send()
		{
			if (!Function || !bValid)
			{
				return false;
			}
			Target->ProcessEvent(Function, Params.Num() > 0 ? Params.GetData() : nullptr);
			return true;
		}

	private:
		UObject* Target = nullptr;
		UFunction* Function = nullptr;
		TArray<uint8> Params;
		bool bValid = true;
	};
}

#endif // WITH_AUTOMATION_TESTS
