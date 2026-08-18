#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AssemblyMortiseActor.generated.h"

class UAssemblyTargetComponent;
class USceneComponent;
class UStaticMeshComponent;

/** Fixed assembly actor whose mesh transform is also the final transform for a movable part. */
UCLASS(Blueprintable)
class GRADUATIONPROJECT_API AAssemblyMortiseActor : public AActor
{
	GENERATED_BODY()

public:
	AAssemblyMortiseActor();
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Assembly")
	UAssemblyTargetComponent* GetAssemblyTargetComponent() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assembly")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Assign the mortise mesh in a blueprint child or on a placed instance. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assembly")
	TObjectPtr<UStaticMeshComponent> MortiseMesh;

	/** Kept at identity relative to MortiseMesh so both components share the terminal transform. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assembly")
	TObjectPtr<UAssemblyTargetComponent> TargetAnchor;

	/** The mesh used only as the semi-transparent correct-volume guide. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assembly|Feedback")
	TObjectPtr<UStaticMeshComponent> GuidanceVolume;

	UFUNCTION(BlueprintPure, Category = "Assembly|Feedback")
	UStaticMeshComponent* GetGuidanceVolume() const { return GuidanceVolume; }
};
