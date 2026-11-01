// Copyright Daniel Raquel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

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
}

#endif // WITH_AUTOMATION_TESTS
