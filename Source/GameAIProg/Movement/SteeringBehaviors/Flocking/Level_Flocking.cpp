// Fill out your copyright notice in the Description page of Project Settings.


#include "Level_Flocking.h"


// Sets default values
ALevel_Flocking::ALevel_Flocking()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_Flocking::BeginPlay()
{
	Super::BeginPlay();

	TrimWorld->SetTrimWorldSize(3000.f);
	TrimWorld->bShouldTrimWorld = true;

	pFlock = TUniquePtr<Flock>(
		new Flock(
			GetWorld(),
			SteeringAgentClass,
			FlockSize,
			TrimWorld->GetTrimWorldSize(),
            pAgentToEvade,
			true)
			);

    // If no agent was assigned in editor, spawn one to test evasion
    if (!pAgentToEvade && SteeringAgentClass)
    {
        const FVector SpawnLoc = FVector(500.f, 0.f, 100.f);
        ASteeringAgent* pAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, SpawnLoc, FRotator::ZeroRotator);
        if (pAgent)
        {
            pAgentToEvade = pAgent;
            bSpawnedEvadeAgent = true;
            // give the evader a simple wander behavior so it moves
            auto pWander = new Wander();
            pWander->SetWanderOffset(100.f);
            pWander->SetWanderRadius(50.f);
            pAgent->SetSteeringBehavior(pWander);
            pEvadeAgentBehavior = pWander;

            // inform the flock about the newly spawned agent
            if (pFlock)
                pFlock->SetAgentToEvade(pAgentToEvade);
        }
    }
    // always hide the spawned evader initially from debug render (toggleable in UI)
    if (pAgentToEvade)
        pAgentToEvade->SetDebugRenderingEnabled(false);
}

// Called every frame
void ALevel_Flocking::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	pFlock->ImGuiRender(WindowPos, WindowSize);
	pFlock->Tick(DeltaTime);
	pFlock->RenderDebug();
	if (bUseMouseTarget)
		pFlock->SetTarget_Seek(MouseTarget);
}

