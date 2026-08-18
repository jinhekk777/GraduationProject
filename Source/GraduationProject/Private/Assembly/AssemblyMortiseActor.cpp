#include "Assembly/AssemblyMortiseActor.h"

#include "Assembly/AssemblyTargetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AAssemblyMortiseActor::AAssemblyMortiseActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MortiseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MortiseMesh"));
	MortiseMesh->SetupAttachment(SceneRoot);
	MortiseMesh->SetSimulatePhysics(false);
	MortiseMesh->SetEnableGravity(false);

	TargetAnchor = CreateDefaultSubobject<UAssemblyTargetComponent>(TEXT("AssemblyTarget"));
	TargetAnchor->SetupAttachment(MortiseMesh);

	GuidanceVolume = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AssemblyGuidanceVolume"));
	GuidanceVolume->SetupAttachment(TargetAnchor);
	GuidanceVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GuidanceVolume->SetCastShadow(false);
	GuidanceVolume->SetVisibility(false);
	TargetAnchor->SetGuidanceMesh(GuidanceVolume);
}

void AAssemblyMortiseActor::BeginPlay()
{
	Super::BeginPlay();
	TargetAnchor->SetGuidanceMesh(GuidanceVolume);
}

UAssemblyTargetComponent* AAssemblyMortiseActor::GetAssemblyTargetComponent() const
{
	return TargetAnchor;
}
