#include "Assembly/AssemblyTenonActor.h"

#include "Assembly/AssemblyMortiseActor.h"
#include "Assembly/AssemblyPartComponent.h"
#include "Assembly/AssemblyTargetComponent.h"
#include "Components/StaticMeshComponent.h"

AAssemblyTenonActor::AAssemblyTenonActor()
{
	TenonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TenonMesh"));
	SetRootComponent(TenonMesh);

	AssemblyPart = CreateDefaultSubobject<UAssemblyPartComponent>(TEXT("AssemblyPart"));
	AssemblyPart->ControlledComponent = TenonMesh;
	AssemblyPart->bAutoSnapWhenInRange = true;
}

void AAssemblyTenonActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureTargetFromMortise();
}

void AAssemblyTenonActor::NotifyGrabStarted()
{
	AssemblyPart->NotifyGrabStarted();
}

void AAssemblyTenonActor::NotifyGrabEnded()
{
	AssemblyPart->NotifyGrabEnded();
}

void AAssemblyTenonActor::ResetAssembly()
{
	AssemblyPart->ResetAssembly();
}

void AAssemblyTenonActor::SetMortiseActor(AAssemblyMortiseActor* InMortiseActor)
{
	MortiseActor = InMortiseActor;
	ConfigureTargetFromMortise();
}

void AAssemblyTenonActor::ConfigureTargetFromMortise()
{
	AssemblyPart->TargetComponent = MortiseActor ? MortiseActor->GetAssemblyTargetComponent() : nullptr;
}
